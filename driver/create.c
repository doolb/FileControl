#include "minidriver.h"

FLT_PREOP_CALLBACK_STATUS
miniPreCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
miniPostCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){

	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	NTSTATUS status = checkPermission(_data, _fltObjects, FALSE);
	if (!NT_SUCCESS(status)) return STATUS_ACCESS_DENIED;

	return FLT_POSTOP_FINISHED_PROCESSING;
}