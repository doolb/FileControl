#include "minidriver.h"
#include "permission.h"
#include "op.h"

FLT_PREOP_CALLBACK_STATUS miniPreWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;

	NTSTATUS status = STATUS_SUCCESS;
	status = opPreCheck(_fltObjects);
	if (!NT_SUCCESS(status)) return FLT_PREOP_SUCCESS_NO_CALLBACK;

	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;

	try{
		ULONG len = iopb->Parameters.Write.Length;
		if (len == 0) leave;

		//
		// check user permission
		//
		status = checkPermission(_data, _fltObjects, TRUE);
		if (!NT_SUCCESS(status)){ loge((NAME"check file permision failed. %x \n", status)); leave; }
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;

		// modify the write offset
		iopb->Parameters.Write.ByteOffset.QuadPart += PM_SIZE;
		FltSetCallbackDataDirty(_data);
		logw((NAME"hide file size in write : %d \n", PM_SIZE));
	}
	finally	{
	}

	if (!NT_SUCCESS(status)) { _data->IoStatus.Status = status; _data->IoStatus.Information = 0; return FLT_PREOP_COMPLETE; }

	logfo;
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS miniPostWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
