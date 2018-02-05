#include "minidriver.h"
#include "permission.h"

FLT_PREOP_CALLBACK_STATUS miniPreQueryInfo(_Inout_ PFLT_CALLBACK_DATA _data,
	_In_ PCFLT_RELATED_OBJECTS _fltObjects,
	_In_opt_ PVOID *_completionContext)
{
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS miniPostQueryInfo(_Inout_ PFLT_CALLBACK_DATA _data,
	_In_ PCFLT_RELATED_OBJECTS _fltObjects,
	_In_opt_ PVOID *_completionContext,
	_In_ FLT_POST_OPERATION_FLAGS _flags)
{
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_completionContext);

	NTSTATUS status = checkPermission(_data, _fltObjects, FALSE);

	if (status == FLT_NO_NEED || status == FLT_ON_DIR) return FLT_POSTOP_FINISHED_PROCESSING;

	if (!NT_SUCCESS(status)) return status;

	//
	// get volume context, because we need the head size
	//
	PVolumeContext ctx = NULL;
	status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
	if (!NT_SUCCESS(status)){ loge((NAME"FltGetVolumeContext failed. %x \n", status)); return status; }

	//
	// modify the file size 
	//
	PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->QueryFileInformation.FileInformationClass == FileStandardInformation &&
		param->QueryFileInformation.Length >= sizeof(FILE_STANDARD_INFORMATION) &&
		param->QueryFileInformation.InfoBuffer != NULL){
		PFILE_STANDARD_INFORMATION info = (PFILE_STANDARD_INFORMATION)param->QueryFileInformation.InfoBuffer;

		// hide the header size
		if (info->EndOfFile.QuadPart >= ctx->PmHeadSize){

			info->EndOfFile.QuadPart -= ctx->PmHeadSize;
			FltSetCallbackDataDirty(_data);
			logi((NAME"query FileStandardInformation, hide size: %d \n", ctx->PmHeadSize));
		}
	}

	//PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->QueryFileInformation.FileInformationClass == FileAllInformation &&
		param->QueryFileInformation.Length >= sizeof(FILE_ALL_INFORMATION) &&
		param->QueryFileInformation.InfoBuffer != NULL){
		PFILE_ALL_INFORMATION info = (PFILE_ALL_INFORMATION)param->QueryFileInformation.InfoBuffer;

		// hide the header size
		if (info->StandardInformation.EndOfFile.QuadPart >= ctx->PmHeadSize){

			info->StandardInformation.EndOfFile.QuadPart -= ctx->PmHeadSize;
			FltSetCallbackDataDirty(_data);
			logi((NAME"query FileAllInformation, hide size: %d \n", ctx->PmHeadSize));
		}

	}
	if (ctx) FltReleaseContext(ctx);
	return FLT_POSTOP_FINISHED_PROCESSING;
}