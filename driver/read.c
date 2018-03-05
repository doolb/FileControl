#include "minidriver.h"
#include "permission.h"
#include "op.h"

FLT_PREOP_CALLBACK_STATUS miniPreRead(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);


	NTSTATUS status = STATUS_SUCCESS;
	status = opPreCheck(_fltObjects);
	if (!NT_SUCCESS(status)) return FLT_PREOP_SUCCESS_NO_CALLBACK;

	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;

	ULONG readlen = iopb->Parameters.Read.Length;

	try
	{
		// is need read
		if (readlen == 0) leave;

		status = checkPermission(_data, _fltObjects, FALSE);
		if (!NT_SUCCESS(status)){ loge((NAME"check file permision failed. %x \n", status)); leave; }
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;


		// modify the read offset
		iopb->Parameters.Read.ByteOffset.QuadPart += PM_SIZE;
		FltSetCallbackDataDirty(_data);
		logw((NAME"hide file size in read : %d \n", PM_SIZE));
	}
	finally
	{
	}

	if (!NT_SUCCESS(status)) { _data->IoStatus.Status = status; _data->IoStatus.Information = 0; return FLT_PREOP_COMPLETE; }

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS miniPostRead(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
