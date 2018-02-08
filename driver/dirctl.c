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


	NTSTATUS status = checkFltStatus(_data, _fltObjects);
	if (status == FLT_NO_NEED) return FLT_POSTOP_FINISHED_PROCESSING;
	if (!NT_SUCCESS(status)) { _data->IoStatus.Status = status; _data->IoStatus.Information = 0; return FLT_PREOP_COMPLETE; }

	//
	// get volume context, because we need the head size
	//
	PVolumeContext ctx;
	status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
	if (!NT_SUCCESS(status)){ loge((NAME"FltGetVolumeContext failed. %x \n", status)); return status; }

	//
	// modify the file size 
	//
	BOOL ismod = FALSE;
	PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->DirectoryControl.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation &&
		param->DirectoryControl.QueryDirectory.Length >= sizeof(FILE_ID_BOTH_DIR_INFORMATION) &&
		param->DirectoryControl.QueryDirectory.DirectoryBuffer != NULL){
		PFILE_ID_BOTH_DIR_INFORMATION info = (PFILE_ID_BOTH_DIR_INFORMATION)param->DirectoryControl.QueryDirectory.DirectoryBuffer;

		if (info->FileIndex == 0){ // is info valid
#pragma warning(disable:4127)
			while (TRUE){
				// is file name valid
				if (info->FileNameLength == 0) goto _next_file_;
				// large file ,we skip it now
				if (info->AllocationSize.HighPart > 0) goto _next_file_;
				// is size valid 
				if (info->EndOfFile.QuadPart < ctx->PmHeadSize) goto _next_file_;

				// hide the header size
				info->EndOfFile.QuadPart -= ctx->PmHeadSize;
				ismod = TRUE;

				// goto next file
			_next_file_:
				if (info->NextEntryOffset == 0) break;
				ULONG ptr = ((ULONG)info) + info->NextEntryOffset;

				info = (PFILE_ID_BOTH_DIR_INFORMATION)ptr;
			}
			if (ismod){
				FltSetCallbackDataDirty(_data);
				logi((NAME"dir FileIdBothDirectoryInformation, hide size: %d \n", ctx->PmHeadSize));
			}			
		}
	}
	ismod = FALSE;
	//PFLT_PARAMETERS param = &_data->Iopb->Parameters;
	if (param->DirectoryControl.QueryDirectory.FileInformationClass == FileBothDirectoryInformation &&
		param->DirectoryControl.QueryDirectory.Length >= sizeof(FILE_BOTH_DIR_INFORMATION) &&
		param->DirectoryControl.QueryDirectory.DirectoryBuffer != NULL){
		PFILE_BOTH_DIR_INFORMATION info = (PFILE_BOTH_DIR_INFORMATION)param->DirectoryControl.QueryDirectory.DirectoryBuffer;

		if (info->FileIndex == 0){ // is info valid
#pragma warning(disable:4127)
			while (TRUE){
				// is file name valid
				if (info->FileNameLength == 0) goto _next_file_b_;
				// large file ,we skip it now
				if (info->AllocationSize.HighPart > 0) goto _next_file_b_;
				// is size valid 
				if (info->EndOfFile.QuadPart < ctx->PmHeadSize) goto _next_file_b_;

				// hide the header size
				info->EndOfFile.QuadPart -= ctx->PmHeadSize;
				ismod = TRUE;

				// goto next file
			_next_file_b_:
				if (info->NextEntryOffset == 0) break;
				ULONG ptr = ((ULONG)info) + info->NextEntryOffset;

				info = (PFILE_BOTH_DIR_INFORMATION)ptr;
			}
			if (ismod){
				FltSetCallbackDataDirty(_data);
				logi((NAME"dir FileBothDirectoryInformation, hide size: %d \n", ctx->PmHeadSize));
			}
		}
	}

	if (ctx) FltReleaseContext(ctx);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
