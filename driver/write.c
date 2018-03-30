#include "minidriver.h"
#include "permission.h"
#include "op.h"

extern NPAGED_LOOKASIDE_LIST gPre2PostContexList;

FLT_PREOP_CALLBACK_STATUS miniPreWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;

	NTSTATUS status = STATUS_SUCCESS;
	status = opPreCheck(_fltObjects);
	if (!NT_SUCCESS(status)) return FLT_PREOP_SUCCESS_NO_CALLBACK;

	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;

	PPre2PostContext p2pCtx = NULL;
	PVolumeContext ctx = NULL;

	PVOID orgBuf = NULL;
	PVOID newBuf = NULL;
	PMDL newMdl = NULL;

	ULONG writelen = iopb->Parameters.Write.Length;

	try{
		if (writelen == 0) leave;

		//
		// check user permission
		//
		status = checkPermission(_data, _fltObjects, FALSE);
		if (!NT_SUCCESS(status)){ loge((NAME"check file permision failed. %x \n", status)); leave; }
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;

		// get volume context
		status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
		if (!NT_SUCCESS(status)){ loge((NAME"get volume context failed. %x", status)); leave; }

		// If this is a non-cached I/O we need to round the length up to the sector size for this device.
		if (FlagOn(IRP_NOCACHE, iopb->IrpFlags)){
			writelen = ROUND_TO_SIZE(writelen, ctx->SectorSize);
		}

		// allocate new buffer
		newBuf = FltAllocatePoolAlignedWithTag(_fltObjects->Instance, NonPagedPool, writelen, BUFFER_SWAP_TAG);
		if (newBuf == NULL){ loge((NAME"allocate memory failed.")); leave; }

		//  We only need to build a MDL for IRP operations.
		if (FlagOn(FLTFL_CALLBACK_DATA_IRP_OPERATION, iopb->IrpFlags)){
			newMdl = IoAllocateMdl(newBuf, writelen, FALSE, FALSE, NULL);
			if (newMdl == NULL){ loge((NAME"allocate mdl failed.")); leave; }
			MmBuildMdlForNonPagedPool(newMdl);
		}

		//  If the users original buffer had a MDL, get a system address.
		if (iopb->Parameters.Write.MdlAddress){
			FLT_ASSERT(((PMDL)iopb->Parameters.Write.MdlAddress)->Next == NULL);
			orgBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Write.MdlAddress, NormalPagePriority);
			if (orgBuf == NULL){
				loge((NAME"get system address for mdl failed."));

				_data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				_data->IoStatus.Information = 0;
				retValue = FLT_PREOP_COMPLETE;
				leave;
			}
		}
		else{
			orgBuf = iopb->Parameters.Write.WriteBuffer;
		}

		//
		//  Copy the memory, we must do this inside the try/except because we
		//  may be using a users buffer address
		//
		try{
			RtlCopyMemory(newBuf, orgBuf, writelen);
			PUCHAR ptr = newBuf;
			if (!IUtil->isMetaData(ptr, writelen)){
				_data->Iopb->Parameters.Write.ByteOffset.QuadPart += PM_SIZE;
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER){
			//  The copy failed, return an error, failing the operation.
			_data->IoStatus.Status = GetExceptionCode();
			_data->IoStatus.Information = 0;
			retValue = FLT_PREOP_COMPLETE;

			loge((NAME":            %wZ Invalid user buffer, oldB=%p, status=%x\n",
				&ctx->Name, orgBuf, _data->IoStatus.Status));

			leave;
		}

		// allocate a p2p context
		p2pCtx = ExAllocateFromNPagedLookasideList(&gPre2PostContexList);
		if (p2pCtx == NULL){ loge((NAME"allocate p2p context failed. %wZ", &ctx->Name)); leave; }

		// set new buffer
		logi((NAME":            %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
			&ctx->Name, newBuf, newMdl, iopb->Parameters.Write.WriteBuffer, iopb->Parameters.Write.MdlAddress, writelen));

		iopb->Parameters.Write.WriteBuffer = newBuf;
		iopb->Parameters.Write.MdlAddress = newMdl;
		iopb->Parameters.Write.ByteOffset.QuadPart += PM_SIZE;
		FltSetCallbackDataDirty(_data);

		//
		//  Pass state to our post-operation callback.
		//
		p2pCtx->SwapBuffer = newBuf;
		p2pCtx->Context = ctx;
		*_completionContext = p2pCtx;

		//
		//  Return we want a post-operation callback
		//
		retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	finally	{
		if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK){
			if (newBuf){ FltFreePoolAlignedWithTag(_fltObjects->Instance, newBuf, BUFFER_SWAP_TAG); newBuf = NULL; }
			if (newMdl){ IoFreeMdl(newMdl); newMdl = NULL; }
			if (ctx){ FltReleaseContext(ctx); ctx = NULL; }
			if (p2pCtx){ ExFreeToNPagedLookasideList(&gPre2PostContexList, p2pCtx); }
		}
	}

	logfo;
	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS miniPostWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID _completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_flags);

	PPre2PostContext p2pCtx = _completionContext;

	logi((NAME":           %wZ newB=%p info=%Iu Freeing\n", &p2pCtx->Context->Name, p2pCtx->SwapBuffer, _data->IoStatus.Information));

	//
	//  Free allocate POOL and volume context
	//
	FltFreePoolAlignedWithTag(_fltObjects->Instance, p2pCtx->SwapBuffer, BUFFER_SWAP_TAG);

	FltReleaseContext(p2pCtx->Context);

	ExFreeToNPagedLookasideList(&gPre2PostContexList, p2pCtx);

	return FLT_POSTOP_FINISHED_PROCESSING;
}