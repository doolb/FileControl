#include "minidriver.h"
#include "permission.h"
#include "op.h"

#define OVERWRITE (FILE_OVERWRITE << 24)

FLT_PREOP_CALLBACK_STATUS
miniPreCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	logfi;
	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;

	NTSTATUS status = STATUS_SUCCESS;
	status = opPreCheck(_fltObjects);
	if (!NT_SUCCESS(status)) { logfo; return FLT_PREOP_SUCCESS_NO_CALLBACK; }


	//
	// overwrite will clean all data in file, so we remove it
	//
	if (iopb->Parameters.Create.Options & OVERWRITE){
		iopb->Parameters.Create.Options &= ~OVERWRITE;
		FltSetCallbackDataDirty(_data);
	}
	logfo;
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
miniPostCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){

	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;

	checkPermission(_data, _fltObjects, FALSE);

	logfo;
	return FLT_POSTOP_FINISHED_PROCESSING;
}