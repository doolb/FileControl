#include "minidriver.h"
#include "permission.h"
#include "checksum.h"

// {5F1E1BC3-4AD5-49D5-A6EE-0A2F86029A64}
static GUID gUserId =
{ 0x5f1e1bc3, 0x4ad5, 0x49d5, { 0xa6, 0xee, 0xa, 0x2f, 0x86, 0x2, 0x9a, 0x64 } };

// {076577A6-C4D9-41D2-970F-1EC90F493DB3}
static GUID gGroupId =
{ 0x76577a6, 0xc4d9, 0x41d2, { 0x97, 0xf, 0x1e, 0xc9, 0xf, 0x49, 0x3d, 0xb3 } };

//
// free the memory of permission data
//
void freePermission(PCFLT_RELATED_OBJECTS _obj,PPermission pm){
	FLT_ASSERT(_obj);
	if (pm)  FltFreePoolAlignedWithTag(_obj->Instance, pm, NAME_TAG); pm = NULL;
}

NTSTATUS setPermission(PCFLT_RELATED_OBJECTS obj, PPermission pm){

	// writh at head
	ULONG retlen;
	LARGE_INTEGER offset = { 0 };

	ASSERT(obj && pm && pm->sizeOnDisk > 0);

	//
	// calc checksum,use crc32
	//
	pm->crc32 = crc_32((PUCHAR)pm, PM_DATA_SIZE);

	//
	// write data
	//
	NTSTATUS status = FltWriteFile(obj->Instance, obj->FileObject, &offset, pm->sizeOnDisk, pm, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);

	if (!NT_SUCCESS(status)){ loge((NAME"write permission to file failed. %x \n", status)); }

	return status;

}

NTSTATUS setDefaultPermission(PCFLT_RELATED_OBJECTS obj, PPermission pm, BOOLEAN rewrite){

	ASSERT(pm != NULL && pm->sizeOnDisk > 0);

	pm->_head = 'FCHD';
	pm->code = PC_Default;
	pm->uid = gUserId;
	pm->gid = gGroupId;


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
		buff = FltAllocatePoolAlignedWithTag(obj->Instance, NonPagedPool, len, NAME_TAG);
		if (!buff){ loge((NAME"allocate buffer failed. \n")); return STATUS_INSUFFICIENT_RESOURCES; }

		// read file
		status = FltReadFile(obj->Instance, obj->FileObject, &offset, len, buff,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status)){ loge((NAME"read file failed. %x \n", status)); goto _set_default_pem_end_; }

	}

	//
	// write permission information
	//
	status = setPermission(obj, pm);
	if (!NT_SUCCESS(status)){ loge((NAME"set file permission failed. %x \n", status)); goto _set_default_pem_end_; }

	//
	// write the origin data
	//
	if (fileInfo.EndOfFile.QuadPart > 0 && !rewrite){
		offset.LowPart = sizeof(Permission);
		status = FltWriteFile(obj->Instance, obj->FileObject, &offset, fileInfo.EndOfFile.LowPart, buff, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);

	}

	//
	// get the new information
	//
	status = FltQueryInformationFile(obj->Instance, obj->FileObject, &fileInfo, sizeof(fileInfo), FileStandardInformation, &retlen);
	if (!NT_SUCCESS(status)){ loge((NAME"get file info failed. %x \n", status)); return status; }
	logi((NAME"file size : %d(%d) \n", fileInfo.EndOfFile, fileInfo.AllocationSize));

_set_default_pem_end_:
	if (buff) { FltFreePoolAlignedWithTag(obj->Instance, buff, NAME_TAG); }
	return status;
}

//
// get the permission of current user, the caller must support the buffer
//
NTSTATUS getPermission(PCFLT_RELATED_OBJECTS _obj, PPermission *_pm) {

	FLT_ASSERT(_obj);

	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER offset = { 0 };
	PVolumeContext ctx = NULL;
	PPermission pm = NULL;
	ULONG retlen;
	*_pm = NULL;

	try{
		//
		// get volume context, so we can get the permission data size on disk
		//
		status = FltGetVolumeContext(_obj->Filter, _obj->Volume, &ctx);
		if (!NT_SUCCESS(status)) { loge((NAME"FltGetVolumeContext failed. %x \n", status)); leave; }
		if (ctx->PmHeadSize == 0){ loge((NAME"permission data size invalid. \n")); status = STATUS_INVALID_PARAMETER; leave; }

		//
		// allocate memory
		//
		pm = FltAllocatePoolAlignedWithTag(_obj->Instance, NonPagedPool, ctx->PmHeadSize, NAME_TAG);
		if (!pm){ loge((NAME"FltAllocatePoolAlignedWithTag failed. \n")); status = STATUS_INVALID_PARAMETER; leave; }
		memset(pm, 0, ctx->PmHeadSize);
		pm->sizeOnDisk = ctx->PmHeadSize;

		//
		// read file information
		//
		status = FltReadFile(_obj->Instance, _obj->FileObject, &offset, pm->sizeOnDisk, pm,
			FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &retlen, NULL, NULL);
		if (!NT_SUCCESS(status)){ loge((NAME"Read file failed. %x \n", status)); }

		//
		// is this controled by ourself
		//
		if (!retlen || retlen < pm->sizeOnDisk || pm->_head != 'FCHD') {
			logw((NAME"no control information in file, write the default permission \n"));

			status = setDefaultPermission(_obj, pm, FALSE);
			if (!NT_SUCCESS(status)){ loge((NAME"set default permission to file failed. %x \n", status)); leave; }
		}

		//
		// checksum
		//
		UINT32 crc32 = crc_32((PUCHAR)pm, PM_DATA_SIZE);
		if (crc32 != pm->crc32){
			logw((NAME"check-sum failed, write the default permission \n"));

			status = setDefaultPermission(_obj, pm, TRUE);
			if (!NT_SUCCESS(status)){ loge((NAME"set default permission to file failed. %x \n", status)); leave; }
		}
	}
	finally{
		if (ctx) FltReleaseContext(ctx); ctx = NULL;

		// if failed, free the buffer
		if (!NT_SUCCESS(status)) freePermission(_obj, pm);
		else *_pm = pm;
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
		status = getPermission(_obj, &pm);
		if (!NT_SUCCESS(status)) { loge((NAME"getPermission failed. %x \n", status)); leave; }


		//
		// is current user
		//
		if (memcmp(&pm->uid, &gUserId, sizeof(GUID)) == 0) {
			if (iswrite)		status = pm->code & PC_User_Write ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
			else				status = pm->code & PC_User_Read ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
		}
		//
		// is the same group
		//
		else if (memcmp(&pm->gid, &gGroupId, sizeof(GUID)) == 0) {
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
	}
	finally{
		freePermission(_obj, pm);
	}
	return status;
}

//
// check the file status, is it is a dir and is need be filter
//
NTSTATUS checkFltStatus(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj){

	NTSTATUS status = FLT_NO_NEED;

	// save volume guid
	UNICODE_STRING guid; WCHAR _guid_buffer[256];	RtlInitEmptyUnicodeString(&guid, _guid_buffer, sizeof(_guid_buffer));

	//
	// get file name information
	//
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	try{
		status = FltGetFileNameInformation(_data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
		// get file name failed when file is in creating
		if (status == STATUS_FLT_INVALID_NAME_REQUEST) { status = FLT_NO_NEED; leave; }
		if (!NT_SUCCESS(status)) { loge((NAME"get file name info failed. %x \n", status)); leave; }

		status = FltParseFileNameInformation(nameInfo);
		if (!NT_SUCCESS(status)) { loge((NAME"parse file name info failed. %x \n", status)); leave; }

		status = FltGetVolumeGuidName(_obj->Volume, &guid, NULL);
		if (!NT_SUCCESS(status)) { loge((NAME"get volume guid failed. %x \n", status)); leave; }

		// is the volume for work
		if (!wcsstr(gWorkRoot.Buffer, guid.Buffer)) { status = FLT_NO_NEED; leave; }

		// is dir
		if (nameInfo->FinalComponent.Length == 0) { status = FLT_ON_DIR; leave; }

		// we need filter it
		status = FLT_NEED;
	}
	finally{
		if (nameInfo) {
			logi((NAME"get file status: %wZ, %x \n", &nameInfo->Name, status));
			FltReleaseFileNameInformation(nameInfo);
		}
	}
	return status;
}