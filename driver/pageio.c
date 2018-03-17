#include "minidriver.h"
#include "permission.h"
#include "op.h"

FLT_PREOP_CALLBACK_STATUS miniPreAcqSection(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;

	NTSTATUS status = STATUS_SUCCESS;
	status = opPreCheck(_fltObjects);
	if (!NT_SUCCESS(status)) return FLT_PREOP_SUCCESS_NO_CALLBACK;

	try{
		//
		// we only can fail this operation when SyncType is set to SyncTypeCreateSection
		// ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/flt-parameters-for-irp-mj-acquire-for-section-synchronization
		//
		PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;
		if (iopb->Parameters.AcquireForSectionSynchronization.SyncType == SyncTypeOther){
			iopb->Parameters.AcquireForSectionSynchronization.SyncType = SyncTypeCreateSection;
			FltSetCallbackDataDirty(_data);
		}

		//NTSTATUS status = checkPermission(_data, _fltObjects, FALSE);
		//if (status == FLT_NO_NEED || status == FLT_ON_DIR) return FLT_POSTOP_FINISHED_PROCESSING;
		//if (!NT_SUCCESS(status)){ _data->IoStatus.Status = status; _data->IoStatus.Information = 0; return FLT_PREOP_COMPLETE; }
	}
	finally{

	}

	//
	// disable File Mapping on the volume 
	//
	loge((NAME"Diable PAGE IO here."));
	_data->IoStatus.Status = STATUS_ACCESS_DENIED;
	_data->IoStatus.Information = 0;

	logfo;
	return FLT_PREOP_COMPLETE;
}