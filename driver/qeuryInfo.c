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

	NTSTATUS status = checkFltStatus(_data, _fltObjects);

	if (status == FLT_NO_NEED || status == FLT_ON_DIR) return FLT_POSTOP_FINISHED_PROCESSING;

	if (!NT_SUCCESS(status)) { _data->IoStatus.Status = status; _data->IoStatus.Information = 0; return FLT_PREOP_COMPLETE; }

	//
	// modify the file size 
	//
	PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->QueryFileInformation.FileInformationClass == FileStandardInformation &&
		param->QueryFileInformation.Length >= sizeof(FILE_STANDARD_INFORMATION) &&
		param->QueryFileInformation.InfoBuffer != NULL){
		PFILE_STANDARD_INFORMATION info = (PFILE_STANDARD_INFORMATION)param->QueryFileInformation.InfoBuffer;

		// hide the header size
		if (info->EndOfFile.QuadPart >= PM_SIZE){

			info->EndOfFile.QuadPart -= PM_SIZE;
			FltSetCallbackDataDirty(_data);
			logi((NAME"query FileStandardInformation, hide size: %d \n", PM_SIZE));
		}
	}

	//PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->QueryFileInformation.FileInformationClass == FileAllInformation &&
		param->QueryFileInformation.Length >= sizeof(FILE_ALL_INFORMATION) &&
		param->QueryFileInformation.InfoBuffer != NULL){
		PFILE_ALL_INFORMATION info = (PFILE_ALL_INFORMATION)param->QueryFileInformation.InfoBuffer;

		// hide the header size
		if (info->StandardInformation.EndOfFile.QuadPart >= PM_SIZE){

			info->StandardInformation.EndOfFile.QuadPart -= PM_SIZE;
			FltSetCallbackDataDirty(_data);
			logi((NAME"query FileAllInformation, hide size: %d \n", PM_SIZE));
		}

	}
	return FLT_POSTOP_FINISHED_PROCESSING;
}