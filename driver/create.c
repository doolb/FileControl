#include "minidriver.h"

FLT_PREOP_CALLBACK_STATUS
miniPreCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	NTSTATUS status;

	WCHAR name[260] = { 0 };
	//
	// get file name information
	//
	PFLT_FILE_NAME_INFORMATION nameInfo;
	status = FltGetFileNameInformation(_data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	if (NT_SUCCESS(status)){
		status = FltParseFileNameInformation(nameInfo);
		if (NT_SUCCESS(status)){
			RtlCopyMemory(name, nameInfo->Name.Buffer, nameInfo->Name.MaximumLength);
			log((NAME"create file:%ws", name));
			if (wcsstr(name, L"1.txt")){
				loge((NAME"file blocked."));

				FltReleaseFileNameInformation(nameInfo);
				_data->IoStatus.Status = STATUS_ACCESS_DENIED;
				_data->IoStatus.Information = 0;
				return FLT_PREOP_COMPLETE;
			}
		}
		FltReleaseFileNameInformation(nameInfo);
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
miniPostCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){

	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);



	return FLT_POSTOP_FINISHED_PROCESSING;
}