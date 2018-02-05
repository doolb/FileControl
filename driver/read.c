#include "minidriver.h"
#include "permission.h"

FLT_PREOP_CALLBACK_STATUS miniPreRead(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;

	ULONG readlen = iopb->Parameters.Read.Length;
	NTSTATUS status = STATUS_SUCCESS;
	PVolumeContext ctx = NULL;

	try
	{
		// is need read
		if (readlen == 0) leave;

		status = checkPermission(_data, _fltObjects, FALSE);
		if (!NT_SUCCESS(status)){ loge((NAME"check file permision failed. %x", status)); leave; }
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;

		//
		// get volume context, because we need the head size
		//
		status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
		if (!NT_SUCCESS(status)){ loge((NAME"FltGetVolumeContext failed. %x", status)); leave; }


		// modify the read offset
		iopb->Parameters.Read.ByteOffset.QuadPart += ctx->PmHeadSize;
		FltSetCallbackDataDirty(_data);
		logi((NAME"hide file size in read : %d", ctx->PmHeadSize));
	}
	finally
	{
		if (ctx) FltReleaseContext(ctx);
	}

	return NT_SUCCESS(status) ? FLT_PREOP_SUCCESS_WITH_CALLBACK : status;
}
FLT_POSTOP_CALLBACK_STATUS miniPostRead(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
