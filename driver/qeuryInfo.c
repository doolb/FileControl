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

	if (status == FLT_NO_NEED) return FLT_POSTOP_FINISHED_PROCESSING;

	if (!NT_SUCCESS(status)) return status;

	//
	// modify the file size 
	//
	PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->QueryFileInformation.FileInformationClass == FileStandardInformation &&
		param->QueryFileInformation.Length > 0 &&
		param->QueryFileInformation.InfoBuffer != NULL){
		PFILE_STANDARD_INFORMATION info = (PFILE_STANDARD_INFORMATION)param->QueryFileInformation.InfoBuffer;

		// hide the header size
		info->EndOfFile.QuadPart -= sizeof(Permission);
		FltSetCallbackDataDirty(_data);
	}
	return FLT_POSTOP_FINISHED_PROCESSING;
}