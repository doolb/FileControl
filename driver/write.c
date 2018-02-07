#include "minidriver.h"
#include "permission.h"

FLT_PREOP_CALLBACK_STATUS miniPreWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;
	NTSTATUS status = STATUS_SUCCESS;
	PVolumeContext ctx = NULL;

	try{
		ULONG len = iopb->Parameters.Write.Length;
		if (len == 0) leave;

		status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
		if (!NT_SUCCESS(status)){ loge((NAME"FltGetVolumeContext failed. %x \n", status)); leave; }

		//
		// check user permission
		//
		status = checkPermission(_data, _fltObjects, FALSE);
		if (!NT_SUCCESS(status)){ loge((NAME"check file permision failed. %x \n", status)); leave; }
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;

		// modify the write offset
		iopb->Parameters.Write.ByteOffset.QuadPart += ctx->PmHeadSize;
		FltSetCallbackDataDirty(_data);
		logw((NAME"hide file size in write : %d \n", ctx->PmHeadSize));
	}
	finally	{
		if (ctx) FltReleaseContext(ctx);
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS miniPostWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
