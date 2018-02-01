#include "minidriver.h"
#include "permission.h"


// {5F1E1BC3-4AD5-49D5-A6EE-0A2F86029A64}
static GUID gUserId =
{ 0x5f1e1bc3, 0x4ad5, 0x49d5, { 0xa6, 0xee, 0xa, 0x2f, 0x86, 0x2, 0x9a, 0x64 } };

// {076577A6-C4D9-41D2-970F-1EC90F493DB3}
static GUID gGroupId =
{ 0x76577a6, 0xc4d9, 0x41d2, { 0x97, 0xf, 0x1e, 0xc9, 0xf, 0x49, 0x3d, 0xb3 } };


NTSTATUS setPermission(PCFLT_RELATED_OBJECTS obj, PPermission pm){

	// writh at head
	ULONG retlen;
	LARGE_INTEGER offset = { 0 };

	NTSTATUS status = FltWriteFile(obj->Instance, obj->FileObject, &offset, sizeof(Permission), pm, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);

	if (!NT_SUCCESS(status)){ loge((NAME"write permission to file failed. %x", status)); }

	return status;

}

NTSTATUS setDefaultPermission(PCFLT_RELATED_OBJECTS obj, PPermission pm){
	pm->_head = 'FCHD';
	pm->code = PC_Default;
	pm->uid = gUserId;
	pm->gid = gGroupId;
	memset(pm->_pad, 'Pd', PADDING);

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
	status = FltQueryInformationFile(obj->Instance, obj->FileObject, &fileInfo, sizeof(fileInfo), FileStandardInformation, &retlen);
	if (!NT_SUCCESS(status)){ loge((NAME"get file info failed. %x", status)); return status; }
	logi((NAME"file size : %d(%d)", fileInfo.EndOfFile, fileInfo.AllocationSize));

	// is file largest than 4gb
	if (fileInfo.AllocationSize.HighPart > 0){ loge((NAME"file is too large.")); return status; }


	// !!!!!!
	// this will cause memory overflower when read large file,
	// fix it later
	if (fileInfo.EndOfFile.QuadPart > 0){
		// allocate buffer
		len = fileInfo.AllocationSize.LowPart;
		buff = FltAllocatePoolAlignedWithTag(obj->Instance, NonPagedPool, len, NAME_TAG);
		if (!buff){ loge((NAME"allocate buffer failed.")); return STATUS_INSUFFICIENT_RESOURCES; }

		// read file
		status = FltReadFile(obj->Instance, obj->FileObject, &offset, len, buff,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status)){ loge((NAME"read file failed. %x", status)); goto _set_default_pem_end_; }

	}

	//
	// write permission information
	//
	status = setPermission(obj, pm);
	if (!NT_SUCCESS(status)){ loge((NAME"set file permission failed. %x", status)); goto _set_default_pem_end_; }

	//
	// write the origin data
	//
	if (fileInfo.EndOfFile.QuadPart > 0){
		offset.LowPart = sizeof(Permission);
		status = FltWriteFile(obj->Instance, obj->FileObject, &offset, fileInfo.EndOfFile.LowPart, buff, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);

		//
		// get the new information
		//
		status = FltQueryInformationFile(obj->Instance, obj->FileObject, &fileInfo, sizeof(fileInfo), FileStandardInformation, &retlen);
		if (!NT_SUCCESS(status)){ loge((NAME"get file info failed. %x", status)); return status; }
		logi((NAME"file size : %d(%d)", fileInfo.EndOfFile, fileInfo.AllocationSize));
	}

_set_default_pem_end_:
	if (buff) { FltFreePoolAlignedWithTag(obj->Instance, buff, NAME_TAG); }
	return status;
}

NTSTATUS getPermission(PCFLT_RELATED_OBJECTS _obj, BOOLEAN iswrite){


	ULONG retlen;
	NTSTATUS status;
	LARGE_INTEGER offset = { 0 };
	Permission pm;

	//
	// read file information
	//
	status = FltReadFile(_obj->Instance, _obj->FileObject, &offset, sizeof(Permission), &pm,
		FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
	if (!NT_SUCCESS(status)){ loge((NAME"Read file failed. %x", status)); }

	//
	// is this controled by ourself
	//
	if (!retlen || retlen != sizeof(Permission) || pm._head != 'FCHD') {
		logw((NAME"no control information in file, write the default permission"));

		status = setDefaultPermission(_obj, &pm);
		if (!NT_SUCCESS(status)){ return status; }
	}

	//
	// is current user
	//
	if (memcmp(&pm.uid, &gUserId, sizeof(GUID)) == 0) {
		if (iswrite) { return pm.code & PC_User_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED; }
		{ return pm.code & PC_User_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED; }
	}
	//
	// is the same group
	//
	else if (memcmp(&pm.uid, &gUserId, sizeof(GUID)) == 0) {
		if (iswrite) { return pm.code & PC_Group_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED; }
		{ return pm.code & PC_Group_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED; }
	}
	//
	// other user
	//
	else{
		if (iswrite) { return pm.code & PC_Other_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED; }
		{ return pm.code & PC_Other_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED; }
	}
}

//
// check user permission,the function can't be called in pre-create or post-cleanup
//
NTSTATUS checkPermission(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj, BOOLEAN iswrite){

	NTSTATUS status = STATUS_SUCCESS;

	// save volume guid
	UNICODE_STRING guid; WCHAR _guid_buffer[256];	RtlInitEmptyUnicodeString(&guid, _guid_buffer, sizeof(_guid_buffer));

	//
	// get file name information
	//
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	try{
		status = FltGetFileNameInformation(_data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
		if (!NT_SUCCESS(status)) { loge((NAME"get file name info failed. %x", status)); leave; }

		status = FltParseFileNameInformation(nameInfo);
		if (!NT_SUCCESS(status)) { loge((NAME"parse file name info failed. %x", status)); leave; }

		status = FltGetVolumeGuidName(_obj->Volume, &guid, NULL);
		if (!NT_SUCCESS(status)) { loge((NAME"get volume guid failed. %x", status)); leave; }

		// is the volume for work
		if (!wcsstr(gWorkRoot.Buffer, guid.Buffer)) return FLT_NO_NEED;

		// is dir
		if (nameInfo->FinalComponent.Length == 0) return FLT_ON_DIR;

		//
		// get user permisssion
		//
		status = getPermission(_obj, iswrite);
	}
	finally{
		if (nameInfo) FltReleaseFileNameInformation(nameInfo);
		log((NAME"check file:%wZ", &nameInfo->Name));
	}
	return status;
}

#undef PADDING