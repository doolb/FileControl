#include "minidriver.h"
#include "permission.h"
#include "checksum.h"
#include "filter.h"
#include "util.h"


User gUser = {
	L"test user",
	L"test group",
	{ 0x5f1e1bc3, 0x4ad5, 0x49d5, { 0xa6, 0xee, 0xa, 0x2f, 0x86, 0x2, 0x9a, 0x64 } },
	{ 0x76577a6, 0xc4d9, 0x41d2, { 0x97, 0xf, 0x1e, 0xc9, 0xf, 0x49, 0x3d, 0xb3 } }
};

static User defaultUser = {
	L"user",
	L"group",
	{ 0x5f1e1bc3, 0x4ad5, 0x49d5, { 0xa6, 0xee, 0xa, 0x2f, 0x86, 0x2, 0x9a, 0x64 } },
	{ 0x76577a6, 0xc4d9, 0x41d2, { 0x97, 0xf, 0x1e, 0xc9, 0xf, 0x49, 0x3d, 0xb3 } }
};

extern KSPIN_LOCK	gFilterLock;
extern PFLT_FILTER	gFilter;

extern UNICODE_STRING gKeyRoot;
extern UNICODE_STRING gWorkRoot;

//
// lookaside list for permission data
//
NPAGED_LOOKASIDE_LIST gPmLookasideList;
NPAGED_LOOKASIDE_LIST gGuidLookasideList;

//
// free the memory of permission data
//
void freePermission(PPermission pm){
	if (pm)  { ExFreeToNPagedLookasideList(&gPmLookasideList, pm); pm = NULL; }
}


NTSTATUS setPermission(PFLT_INSTANCE ins, PFILE_OBJECT obj, PPermission pm){

	// writh at head
	ULONG retlen;
	LARGE_INTEGER offset = { 0 };

	ASSERT(ins && obj && pm);

	//
	// clear memory
	//
	memset(pm + PM_DATA_SIZE, 0, PM_SIZE - PM_DATA_SIZE);

	//
	// calc checksum,use crc32
	//
	pm->crc32 = crc_32((PUCHAR)pm, PM_DATA_SIZE);

	//
	// encrypt data
	//
	retlen = PM_SIZE;
	PVOID en = IUtil->encrypt((PVOID)pm, &retlen);
	if (!en){ loge((NAME"encrypt data failed.")); return STATUS_INSUFFICIENT_RESOURCES; }
	ASSERT(retlen == PM_SIZE);	// size should be the same

	//
	// write data
	//
	NTSTATUS status = FltWriteFile(ins, obj, &offset, PM_SIZE, en, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);

	if (!NT_SUCCESS(status)){ loge((NAME"write permission to file failed. %x \n", status)); }

	ExFreePoolWithTag(en, UTIL_TAG);

	return status;

}

NTSTATUS setDefaultPermission(PFLT_INSTANCE ins, PFILE_OBJECT obj, PPermission pm, BOOL rewrite){

	ASSERT(pm);

	pm->_head = 'FCHD';
	pm->code = PC_Default;
	pmSetName(pm, L"user");
	pmSetGroup(pm, L"group");
	pm->user.uid = gUser.uid;
	pm->user.gid = gUser.gid;


	NTSTATUS status = STATUS_SUCCESS;
	//LARGE_INTEGER offset = { 0 };
	ULONG retlen;

	FILE_STANDARD_INFORMATION fileInfo;
	LARGE_INTEGER offset = null;
	PVOID buff = NULL;
	ULONG len = 0;

	//
	// save the origin data
	//
	// get file size
	status = FltQueryInformationFile(ins, obj, &fileInfo, sizeof(fileInfo), FileStandardInformation, &retlen);
	if (!NT_SUCCESS(status)){ loge((NAME"get file info failed. %x \n", status)); return status; }
	logi((NAME"file size : %d(%d)", fileInfo.EndOfFile, fileInfo.AllocationSize));

	// is file largest than 4gb
	if (fileInfo.AllocationSize.HighPart > 0){ loge((NAME"file is too large. \n")); return status; }

	//
	// is rewrite the permission data
	//
	//if (rewrite) fileInfo.EndOfFile.QuadPart -= sizeof(Permission);

	// !!!!!!
	// this will cause memory overflower when read large file,
	// fix it later
	if (fileInfo.EndOfFile.QuadPart > 0 && !rewrite){
		// allocate buffer
		len = fileInfo.AllocationSize.LowPart;
		buff = FltAllocatePoolAlignedWithTag(ins, NonPagedPool, len, NAME_TAG);
		if (!buff){ loge((NAME"allocate buffer failed. \n")); return STATUS_INSUFFICIENT_RESOURCES; }

		// read file
		status = FltReadFile(ins, obj, &offset, len, buff,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status)){ loge((NAME"read file failed. %x \n", status)); goto _set_default_pem_end_; }

	}

	//
	// write permission information
	//
	status = setPermission(ins, obj, pm);
	if (!NT_SUCCESS(status)){ loge((NAME"set file permission failed. %x \n", status)); goto _set_default_pem_end_; }

	//
	// write the origin data
	//
	if (fileInfo.EndOfFile.QuadPart > 0 && !rewrite){
		offset.LowPart = sizeof(Permission);
		status = FltWriteFile(ins, obj, &offset, fileInfo.EndOfFile.LowPart, buff, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);

	}

	//
	// get the new information
	//
	status = FltQueryInformationFile(ins, obj, &fileInfo, sizeof(fileInfo), FileStandardInformation, &retlen);
	if (!NT_SUCCESS(status)){ loge((NAME"get file info failed. %x \n", status)); return status; }
	logi((NAME"file size : %d(%d) \n", fileInfo.EndOfFile, fileInfo.AllocationSize));

_set_default_pem_end_:
	if (buff) { FltFreePoolAlignedWithTag(ins, buff, NAME_TAG); }
	return status;
}

//
// get the permission of current user, the caller must support the buffer
//
NTSTATUS getPermission(PFLT_INSTANCE ins, PFILE_OBJECT obj, PPermission *_pm) {

	FLT_ASSERT(ins && obj && _pm);

	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER offset = { 0 };
	PPermission pm = NULL;
	BOOL rewrite = FALSE;
	ULONG retlen = 0;
	*_pm = NULL;


	try{

		//
		// allocate memory
		//
		pm = ExAllocateFromNPagedLookasideList(&gPmLookasideList);
		if (!pm){ loge((NAME"FltAllocatePoolAlignedWithTag failed. \n")); status = STATUS_INVALID_PARAMETER; leave; }
		memset(pm, 0, PM_SIZE);

		//
		// read file information
		//
		status = FltReadFile(ins, obj, &offset, PM_SIZE, pm,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status)){ loge((NAME"Read file failed. %x \n", status)); }

#pragma region check data
		//
		// is this controled by ourself
		//
		if (!retlen || retlen < PM_SIZE) { logw((NAME"no control information in file")); status = FLT_INVALID_HEAD; leave; }

		//
		// decrypt data
		//
		retlen = PM_SIZE;
		PVOID de = IUtil->decrypt((PVOID)pm, &retlen);
		ASSERT(retlen == PM_SIZE);
		memcpy_s(pm, PM_SIZE, de, PM_SIZE);
		ExFreePoolWithTag(de, UTIL_TAG);

		if (pm->_head != 'FCHD'){ logw((NAME"invalid head")); status = FLT_INVALID_HEAD; rewrite = TRUE; leave; }

		//
		// checksum
		//
		UINT32 crc32 = crc_32((PUCHAR)pm, PM_DATA_SIZE);
		if (crc32 != pm->crc32){ logw((NAME"check-sum failed")); status = FLT_INVALID_HEAD; rewrite = TRUE; leave; }

#pragma endregion
	}
	finally{
		//
		// is get permission data success
		//
		if (!NT_SUCCESS(status)){
			logw(("write the default permission \n"));
			status = setDefaultPermission(ins, obj, pm, rewrite);
			if (!NT_SUCCESS(status)){ loge((NAME"set default permission to file failed. %x \n", status)); }
		}

		// if failed, free the buffer
		if (!NT_SUCCESS(status)) freePermission(pm);
		else *_pm = pm;
	}

	return status;
}

NTSTATUS cmpPermission(PPermission pm, BOOL iswrite){
	NTSTATUS status = STATUS_SUCCESS;
	ASSERT(pm);

	//
	// is user login
	//
	if (gKeyRoot.Length == 0) return STATUS_ACCESS_DENIED;

	//
	// is current user
	//
	if (pmIsUser(pm, &gUser)) {
		if (iswrite)		status = pm->code & PC_User_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
		else				status = pm->code & PC_User_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
	}
	//
	// is the same group
	//
	else if (pmIsGroup(pm, &gUser)) {
		if (iswrite)		status = pm->code & PC_Group_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
		else				status = pm->code & PC_Group_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
	}
	//
	// other user
	//
	else{
		if (iswrite)		status = pm->code & PC_Other_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
		else				status = pm->code & PC_Other_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
	}
	return status;
}

//
// check user permission, the file must be opened
// iswrite: is user want to write file
//
NTSTATUS checkPermission(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj, BOOLEAN iswrite){

	NTSTATUS status = STATUS_SUCCESS;
	PPermission pm = NULL;

	try{

		//
		// check file status
		//
		status = checkFltStatus(_data, _obj);
		if (!NT_SUCCESS(status)) { loge((NAME"checkFltStatus failed. %x \n", status)); leave; }
		// is need filter
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;


		//
		// get file permission data
		//
		status = getPermission(_obj->Instance, _obj->FileObject, &pm);
		if (!NT_SUCCESS(status) || !pm) { loge((NAME"getPermission failed. %x \n", status)); leave; }

		//
		// compare user permission
		//
		status = cmpPermission(pm, iswrite);

	}
	finally{
		freePermission(pm);
	}
	return status;
}

//
// check the file status, is it is a dir and is need be filter
//
NTSTATUS checkFltStatus(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj){

	NTSTATUS status = FLT_NO_NEED;

	// save volume guid
	UNICODE_STRING guid = null;

	//
	// get file name information
	//
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	try{
		//
		// is work root has be set
		//
		if (gWorkRoot.Length == 0){ return FLT_NO_NEED; }

		status = FltGetFileNameInformation(_data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
		// get file name failed when file is in creating
		if (status == STATUS_FLT_INVALID_NAME_REQUEST) { status = FLT_NO_NEED; leave; }
		if (!NT_SUCCESS(status)) { loge((NAME"get file name info failed. %x \n", status)); leave; }

		status = FltParseFileNameInformation(nameInfo);
		if (!NT_SUCCESS(status)) { loge((NAME"parse file name info failed. %x \n", status)); leave; }

		guid.Buffer = ExAllocateFromNPagedLookasideList(&gGuidLookasideList);
		if (!guid.Buffer){ loge((NAME"allocate buffer failed")); leave; }
		guid.MaximumLength = 2 * GUID_SIZE * sizeof(WCHAR);
		guid.Length = 0;

		status = FltGetVolumeGuidName(_obj->Volume, &guid, NULL);
		if (!NT_SUCCESS(status)) { loge((NAME"get volume guid failed. %x \n", status)); leave; }

		// is dir
		if (nameInfo->FinalComponent.Length == 0) { status = FLT_ON_DIR; leave; }

		//
		// is need filter now
		//
		status = onfilter(nameInfo, &guid);
		if (status != STATUS_SUCCESS) leave;

		// we need filter it
		status = FLT_NEED;
	}
	finally{
		if (nameInfo) {
			logi((NAME"get file status: %wZ, %x \n", &nameInfo->Name, status));
			FltReleaseFileNameInformation(nameInfo);
		}
		if (guid.Buffer){ ExFreeToNPagedLookasideList(&gGuidLookasideList, guid.Buffer); guid.Buffer = NULL; }
	}
	return status;
}

#pragma region user key

static NTSTATUS readUserKey(PFLT_INSTANCE instance, PUNICODE_STRING path, PUserKey key){
	ASSERT(path);
	ASSERT(key);
	ASSERT(gFilter);

	NTSTATUS status = STATUS_SUCCESS;

	HANDLE hand = NULL;
	PFILE_OBJECT obj = NULL;
	IO_STATUS_BLOCK iostatus = null;

	ULONG retlen = 0;
	LARGE_INTEGER offset = null;


	UNICODE_STRING name;
	short size = path->Length + sizeof(USER_KEY_FILE);
	PWCHAR name_buffer = NULL;

	try{
		//
		// file name
		//
		name_buffer = ExAllocatePoolWithTag(NonPagedPool, size, PM_TAG);
		if (!name_buffer){ loge((NAME"allocate memory failed.")); leave; }
		RtlInitEmptyUnicodeString(&name, name_buffer, size);
		RtlCopyUnicodeString(&name, path);
		RtlAppendUnicodeToString(&name, USER_KEY_FILE);
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		//
		// open file
		//
		status = FltCreateFileEx(gFilter, instance, &hand, &obj, FILE_GENERIC_READ, &oa, &iostatus, NULL,
			FILE_ATTRIBUTE_HIDDEN, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
		if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); leave; }

		//
		// read file
		//
		status = FltReadFile(instance, obj, &offset, sizeof(UserKey), key,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status) || retlen != sizeof(UserKey)){ loge((NAME"read file failed. %x.%wZ", status, &name)); leave; }

	}
	finally{
		if (name_buffer) ExFreePoolWithTag(name_buffer, PM_TAG); name_buffer = NULL;

		if (!NT_SUCCESS(status) && retlen > 0){
			loge((NAME"read user key failed. clear it : %wZ \n", &name));
			FILE_END_OF_FILE_INFORMATION info = { 0 };
			status = FltSetInformationFile(instance, obj, &info, sizeof(info), FileEndOfFileInformation);
			if (!NT_SUCCESS(status)){ loge((NAME"clear file failed. %x.%wZ", status, &name)); }
		}

		FltClose(hand);
	}

	return status;
}
static NTSTATUS writeUserKey(PFLT_INSTANCE instance, PUNICODE_STRING path, PUserKey key){
	ASSERT(path);
	ASSERT(key);

	ASSERT(gFilter);

	NTSTATUS status = STATUS_SUCCESS;


	HANDLE hand = NULL;
	PFILE_OBJECT obj = NULL;
	IO_STATUS_BLOCK iostatus = null;

	ULONG retlen = 0;
	LARGE_INTEGER offset = null;
	LARGE_INTEGER allocate = null;
	UNICODE_STRING name;
	short size = path->Length + sizeof(USER_KEY_FILE);
	PWCHAR name_buffer = NULL;

	try{
		//
		// file name
		//
		name_buffer = ExAllocatePoolWithTag(NonPagedPool, size, PM_TAG);
		if (!name_buffer){ loge((NAME"allocate memory failed.")); leave; }
		RtlInitEmptyUnicodeString(&name, name_buffer, size);
		RtlCopyUnicodeString(&name, path);
		RtlAppendUnicodeToString(&name, USER_KEY_FILE);
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		//
		// open file
		//
		allocate.QuadPart = 4096;
		status = FltCreateFileEx(gFilter, instance, &hand, &obj, FILE_GENERIC_WRITE, &oa, &iostatus, &allocate,
			FILE_ATTRIBUTE_HIDDEN, 0, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
		if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); leave; }

		//
		// write file
		//
		status = FltWriteFile(instance, obj, &offset, sizeof(UserKey), key,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status) || retlen != sizeof(UserKey)){ loge((NAME"write file failed. %x.%wZ", status, &name)); leave; }

	}
	finally{
		if (name_buffer) ExFreePoolWithTag(name_buffer, PM_TAG); name_buffer = NULL;

		if (!NT_SUCCESS(status) && retlen > 0){
			loge((NAME"read user key failed. clear it : %wZ \n", &name));
			FILE_END_OF_FILE_INFORMATION info = { 0 };
			status = FltSetInformationFile(instance, obj, &info, sizeof(info), FileEndOfFileInformation);
			if (!NT_SUCCESS(status)){ loge((NAME"clear file failed. %x.%wZ", status, &name)); }
		}

		FltClose(hand);
	}

	return status;
}


PUserKey registryUserKey(PUNICODE_STRING path, PVOID _reg){
	ASSERT(path);
	ASSERT(_reg);

	PMsg_User_Registry reg = _reg;
	PUserKey key = NULL;

	try{
		//
		// allocate memory
		//
		key = ExAllocatePoolWithTag(NonPagedPool, sizeof(UserKey), PM_TAG);
		if (!key){ loge((NAME"allocate memory failed.")); leave; }
		memset(key, 0, sizeof(UserKey));

		// name
		memcpy_s(&key->user, sizeof(User), &reg->user, sizeof(User));

		// password
		IUtil->hash((uint8_t*)reg->password, wcsnlen(reg->password, PM_NAME_MAX) * sizeof(WCHAR), key->passwd);
	}
	finally{
	}

	return key;
}

NTSTATUS deleteUserKey(PFLT_INSTANCE instance, PUNICODE_STRING path){
	ASSERT(instance);
	ASSERT(gFilter);
	ASSERT(path);


	NTSTATUS status = STATUS_SUCCESS;


	HANDLE hand = NULL;
	PFILE_OBJECT obj = NULL;
	IO_STATUS_BLOCK iostatus = null;

	LARGE_INTEGER allocate = null;
	UNICODE_STRING name;
	short size = path->Length + sizeof(USER_KEY_FILE);
	PWCHAR name_buffer = NULL;

	try{
		//
		// file name
		//
		name_buffer = ExAllocatePoolWithTag(NonPagedPool, size, PM_TAG);
		if (!name_buffer){ loge((NAME"allocate memory failed.")); leave; }
		RtlInitEmptyUnicodeString(&name, name_buffer, size);
		RtlCopyUnicodeString(&name, path);
		RtlAppendUnicodeToString(&name, USER_KEY_FILE);
		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		//
		// open file
		//
		allocate.QuadPart = 4096;
		status = FltCreateFileEx(gFilter, instance, &hand, &obj, FILE_GENERIC_WRITE, &oa, &iostatus, &allocate,
			FILE_ATTRIBUTE_HIDDEN, 0, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
		if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); leave; }

		//
		// clear file
		//
		loge((NAME"read user key failed. clear it : %wZ \n", &name));
		FILE_END_OF_FILE_INFORMATION info = { 0 };
		status = FltSetInformationFile(instance, obj, &info, sizeof(info), FileEndOfFileInformation);
		if (!NT_SUCCESS(status)){ loge((NAME"clear file failed. %x.%wZ", status, &name)); }
	}
	finally{
		if (name_buffer) ExFreePoolWithTag(name_buffer, PM_TAG); name_buffer = NULL;
		FltClose(hand);
	}

	return status;
}

struct _IUserKey IUserKey[1] = {
	readUserKey,
	writeUserKey,
	registryUserKey,
	deleteUserKey
};
#pragma endregion