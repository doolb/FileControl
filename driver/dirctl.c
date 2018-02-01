#include "minidriver.h"
#include "permission.h"

FLT_PREOP_CALLBACK_STATUS miniPreDirCtrl(_Inout_ PFLT_CALLBACK_DATA _data,
	_In_ PCFLT_RELATED_OBJECTS _fltObjects,
	_In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS miniPostDirCtrl(_Inout_ PFLT_CALLBACK_DATA _data,
	_In_ PCFLT_RELATED_OBJECTS _fltObjects,
	_In_opt_ PVOID *_completionContext,
	_In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_completionContext);


	NTSTATUS status = checkPermission(_data, _fltObjects, FALSE);

	if (status == FLT_NO_NEED) return FLT_POSTOP_FINISHED_PROCESSING;

	if (!NT_SUCCESS(status)) return status;

	//
	// modify the file size 
	//
	PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->DirectoryControl.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation &&
		param->DirectoryControl.QueryDirectory.DirectoryBuffer != NULL){
		PFILE_ID_BOTH_DIR_INFORMATION info = (PFILE_ID_BOTH_DIR_INFORMATION)param->DirectoryControl.QueryDirectory.DirectoryBuffer;

		if (info->FileIndex == 0){ // is info valid
#pragma warning(disable:4127)
			while (TRUE){
				// is file name valid
				if (info->FileNameLength == 0) continue;

				// large file ,we skip it now
				if (info->AllocationSize.HighPart > 0) continue;

				// hide the header size
				info->EndOfFile.QuadPart -= sizeof(Permission);

				// goto next file
				if (info->NextEntryOffset == 0) break;
				ULONG ptr = ((ULONG)info) + info->NextEntryOffset;
				info = (PFILE_ID_BOTH_DIR_INFORMATION)ptr;
			}
			FltSetCallbackDataDirty(_data);
		}
	}

	//PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->DirectoryControl.QueryDirectory.FileInformationClass == FileBothDirectoryInformation &&
		param->DirectoryControl.QueryDirectory.DirectoryBuffer != NULL){
		PFILE_BOTH_DIR_INFORMATION info = (PFILE_BOTH_DIR_INFORMATION)param->DirectoryControl.QueryDirectory.DirectoryBuffer;

		if (info->FileIndex == 0){ // is info valid
#pragma warning(disable:4127)
			while (TRUE){
				// is file name valid
				if (info->FileNameLength == 0) continue;

				// large file ,we skip it now
				if (info->AllocationSize.HighPart > 0) continue;

				// hide the header size
				info->EndOfFile.QuadPart -= sizeof(Permission);

				// goto next file
				if (info->NextEntryOffset == 0) break;
				ULONG ptr = ((ULONG)info) + info->NextEntryOffset;
				info = (PFILE_BOTH_DIR_INFORMATION)ptr;
			}
			FltSetCallbackDataDirty(_data);
		}
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}
