#include <fltKernel.h>
#include "msg.h"
#include "filter.h"


NTSTATUS file_get(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;

	//
	// check buffer and size
	//
	if (!buffer || size < sizeof(Msg_File)){ *retlen = sizeof(Msg_File); return STATUS_BUFFER_TOO_SMALL; }

	//
	// has work root
	//
	if (!gInstance) { loge((NAME"no work root \n")); return STATUS_ACCESS_DENIED; }

	//
	// check parameter
	//
	PMsg_File file = buffer;
	if (file->path == NULL){ loge((NAME"invalid file path \n")); return STATUS_INVALID_PARAMETER; }

	//
	// check file path
	//
	if (!wcsstr(file->path, gWorkRoot.Buffer)){ loge((NAME"file isnot in work root \n")); return STATUS_INVALID_PARAMETER; }

	//
	// open file
	//
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, file->path);
	OBJECT_ATTRIBUTES oa = null;
	InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	PFILE_OBJECT obj = NULL;
	HANDLE hand = NULL;
	IO_STATUS_BLOCK iostatus;
	status = FltCreateFileEx(gFilter, gInstance, &hand, &obj, FILE_GENERIC_READ, &oa, &iostatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
	if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); return status; }

	//
	// get permission
	//
	PPermission pm = NULL;
	status = getPermission(gInstance, obj, &pm);
	if (!NT_SUCCESS(status)){ loge((NAME"getPermission failed. %x.%wZ", status, &name)); FltClose(hand); return status; }

	//
	// copy data
	//
	file->pmCode = pm->code;
	memcpy_s(&file->user, sizeof(User), &pm->user, sizeof(User));
	*retlen = sizeof(Msg_File);

	//
	// clear
	//
	freePermission(pm);
	FltClose(hand);
	return status;

}

NTSTATUS file_set(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;

	//
	// check buffer and size
	//
	if (!buffer || size < sizeof(Msg_File)){ *retlen = sizeof(Msg_File); return STATUS_BUFFER_TOO_SMALL; }

	//
	// has work root
	//
	if (!gInstance) { loge((NAME"no work root \n")); return STATUS_ACCESS_DENIED; }

	//
	// check parameter
	//
	PMsg_File file = buffer;
	if (file->path == NULL){ loge((NAME"invalid file path \n")); return STATUS_INVALID_PARAMETER; }

	//
	// check file path
	//
	if (!wcsstr(file->path, gWorkRoot.Buffer)){ loge((NAME"file isnot in work root \n")); return STATUS_INVALID_PARAMETER; }

	//
	// open file
	//
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, file->path);
	OBJECT_ATTRIBUTES oa = null;
	InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	PFILE_OBJECT obj = NULL;
	HANDLE hand = NULL;
	IO_STATUS_BLOCK iostatus;
	status = FltCreateFileEx(gFilter, gInstance, &hand, &obj, FILE_GENERIC_READ, &oa, &iostatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
	if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); return status; }

	//
	// get permission
	//
	PPermission pm = NULL;
	status = getPermission(gInstance, obj, &pm);
	if (!NT_SUCCESS(status)){ loge((NAME"getPermission failed. %x.%wZ", status, &name)); FltClose(hand); return status; }

	//
	// compare permission
	//
	if (!pmIsUser(pm, &gUser)){ loge((NAME"only file Owner can set permission. %wZ", &name)); freePermission(pm); FltClose(hand); return status; }

	//
	// copy data
	//
	pm->code = file->pmCode;
	memcpy_s(&pm->user, sizeof(User), &file->user, sizeof(User));

	//
	// set permission
	//
	setPermission(gInstance, obj, pm);
	//
	// clear
	//
	freePermission(pm);
	FltClose(hand);
	return status;
}