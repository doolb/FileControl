#include "minidriver.h"
#include "permission.h"
#include "op.h"

extern NPAGED_LOOKASIDE_LIST gPre2PostContexList;

FLT_POSTOP_CALLBACK_STATUS miniPostReadSafe(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags);

FLT_PREOP_CALLBACK_STATUS miniPreRead(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;

	NTSTATUS status = STATUS_SUCCESS;
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	status = opPreCheck(_fltObjects);
	if (!NT_SUCCESS(status)) return retValue;

	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;

	ULONG readlen = iopb->Parameters.Read.Length;
	// swap buffer
	PPre2PostContext p2pCtx = NULL;
	PVolumeContext ctx = NULL;
	PVOID newBuf = NULL;
	PMDL newMdl = NULL;

	try
	{
		// is need read
		if (readlen == 0) leave;

		status = checkPermission(_data, _fltObjects, FALSE);
		if (!NT_SUCCESS(status)){ loge((NAME"check file permision failed. %x \n", status)); leave; }
		if (status == FLT_NO_NEED || status == FLT_ON_DIR) leave;

		status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
		if (!NT_SUCCESS(status)){ loge((NAME"get volume context failed. %x", status)); leave; }

		//
		// build swap buffer
		//
		if (FlagOn(IRP_NOCACHE, iopb->IrpFlags)){
			readlen = ROUND_TO_SIZE(readlen, ctx->SectorSize);
		}

		//  Allocate aligned nonPaged memory for the buffer we are swapping
		newBuf = FltAllocatePoolAlignedWithTag(_fltObjects->Instance, NonPagedPool, readlen, BUFFER_SWAP_TAG);
		if (!newBuf){ loge((NAME"allocate new buffer failed. %x", status)); leave; }

		//  We only need to build a MDL for IRP operations.
		if (FlagOn(FLTFL_CALLBACK_DATA_IRP_OPERATION, iopb->IrpFlags)){
			//  Allocate a MDL for the new allocated memory.
			newMdl = IoAllocateMdl(newBuf, readlen, FALSE, FALSE, NULL);
			if (!newMdl){ loge((NAME"allocate new mdl failed. %x", status)); leave; }
			//  setup the MDL for the non-paged pool we just allocated
			MmBuildMdlForNonPagedPool(newMdl);
		}

		// get a pre2Post context structure.
		p2pCtx = ExAllocateFromNPagedLookasideList(&gPre2PostContexList);
		if (p2pCtx == NULL){ loge((NAME"allocate p2p context failed.")); leave; }

		//  Log that we are swapping
		logi((NAME":             %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n", &ctx->Name, newBuf, newMdl, iopb->Parameters.Read.ReadBuffer, iopb->Parameters.Read.MdlAddress, readlen));

		//  Update the buffer pointers and MDL address, mark we have changed something.
		iopb->Parameters.Read.ReadBuffer = newBuf;
		iopb->Parameters.Read.MdlAddress = newMdl;
		iopb->Parameters.Read.ByteOffset.QuadPart += PM_SIZE;
		FltSetCallbackDataDirty(_data);

		//
		//  Pass state to our post-operation callback.
		//
		p2pCtx->SwapBuffer = newBuf;
		p2pCtx->Context = ctx;
		*_completionContext = p2pCtx;


		logi((NAME"hide file size in read : %d \n", PM_SIZE));

		retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	finally
	{
		if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK){
			if (newBuf){ FltFreePoolAlignedWithTag(_fltObjects->Instance, newBuf, BUFFER_SWAP_TAG); newBuf = NULL; }
			if (newMdl){ IoFreeMdl(newMdl); newMdl = NULL; }
			if (ctx){ FltReleaseContext(ctx); ctx = NULL; }
			if (p2pCtx){ ExFreeToNPagedLookasideList(&gPre2PostContexList, p2pCtx); }
		}
	}
	logfo;

	if (!NT_SUCCESS(status)) { _data->IoStatus.Status = status; _data->IoStatus.Information = 0; return FLT_PREOP_COMPLETE; }
	return retValue;
}


FLT_POSTOP_CALLBACK_STATUS miniPostRead(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID _completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_flags);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;

	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	PPre2PostContext p2pCtx = _completionContext;
	PFLT_IO_PARAMETER_BLOCK iopb = _data->Iopb;
	BOOL cleanupBuffer = TRUE;
	PVOID orgBuf = NULL;

	FLT_ASSERT(!FlagOn(_flags, FLTFL_POST_OPERATION_DRAINING));

	try{
		//  If the operation failed or the count is zero, just return 
		if (!NT_SUCCESS(_data->IoStatus.Status) || _data->IoStatus.Information == 0){
			logi((NAME":            %wZ newB=%p No data read, status=%x, info=%Iu\n",
				&p2pCtx->Context->Name, p2pCtx->SwapBuffer, _data->IoStatus.Status, _data->IoStatus.Information));
			leave;
		}

		// copy the read data back into the users buffer.
		// the parameters passed in are for the users original buffers not our swapped buffers.
		if (iopb->Parameters.Read.MdlAddress){
			//  This should be a simple MDL. We don't expect chained MDLs this high up the stack
			FLT_ASSERT(((PMDL)iopb->Parameters.Read.MdlAddress)->Next == NULL);
			orgBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress, NormalPagePriority);
			if (orgBuf == NULL){
				loge((NAME"get system address for mdl failed."));
				_data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				_data->IoStatus.Information = 0;
				leave;
			}
		}
		else if (FlagOn(_data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
			FlagOn(_data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)){
			//
			//  If this is a system buffer, just use the given address because
			//      it is valid in all thread contexts.
			//  If this is a FASTIO operation, we can just use the
			//      buffer (inside a try/except) since we know we are in
			//      the correct thread context (you can't pend FASTIO's).
			//
			orgBuf = iopb->Parameters.Read.ReadBuffer;
		}
		else{
			//
			//  They don't have a MDL and this is not a system buffer
			//  or a fastio so this is probably some arbitrary user
			//  buffer.  We can not do the processing at DPC level so
			//  try and get to a safe IRQL so we can do the processing.
			//
			if (FltDoCompletionProcessingWhenSafe(_data, _fltObjects, _completionContext, _flags,
				miniPostReadSafe, &retValue)){
				cleanupBuffer = FALSE;
			}
			else{
				loge((NAME"unable to post to a safe IRQL."));
				_data->IoStatus.Status = STATUS_UNSUCCESSFUL;
				_data->IoStatus.Information = 0;
			}

			leave;
		}

		//
		//  We either have a system buffer or this is a fastio operation
		//  so we are in the proper context.  Copy the data handling an
		//  exception.
		//
		try{
 			PUCHAR ptr = p2pCtx->SwapBuffer;
// 			if (!IUtil->isMetaData(ptr, _data->IoStatus.Information)){
// 				ptr += PM_SIZE;
// 				_data->IoStatus.Information -= PM_SIZE;
// 			}
			RtlCopyMemory(orgBuf, ptr, _data->IoStatus.Information);
		}
		except(EXCEPTION_EXECUTE_HANDLER){
			//
			//  The copy failed, return an error, failing the operation.
			//
			_data->IoStatus.Status = GetExceptionCode();
			_data->IoStatus.Information = 0;

			loge((":            %wZ Invalid user buffer, oldB=%p, status=%x\n",
				&p2pCtx->Context->Name, orgBuf, _data->IoStatus.Status));
		}
	}
	finally{
		if (cleanupBuffer){
			logi((":            %wZ newB=%p info=%Iu Freeing\n",
				&p2pCtx->Context->Name, p2pCtx->SwapBuffer, _data->IoStatus.Information));

			FltFreePoolAlignedWithTag(_fltObjects->Instance, p2pCtx->SwapBuffer, BUFFER_SWAP_TAG);

			FltReleaseContext(p2pCtx->Context);

			ExFreeToNPagedLookasideList(&gPre2PostContexList, p2pCtx);
		}
	}


	logfo;
	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS miniPostReadSafe(_Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags){

	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	PPre2PostContext p2pCtx = CompletionContext;
	PVOID orgBuf;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	FLT_ASSERT(Data->IoStatus.Information != 0);

	status = FltLockUserBuffer(Data);
	if (!NT_SUCCESS(status)){
		loge((NAME"can't lock user buffer. %x", status));
		Data->IoStatus.Status = status;
		Data->IoStatus.Information = 0;
	}
	else{
		//  Get a system address for this buffer.
		orgBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress, NormalPagePriority);
		if (orgBuf == NULL){
			loge((NAME"fail to get system buffer for mdl."));
			Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			Data->IoStatus.Information = 0;
		}
		else{
			//  Copy the data back to the original buffer.
			PUCHAR ptr = p2pCtx->SwapBuffer;
// 			if (!IUtil->isMetaData(ptr, Data->IoStatus.Information)){
// 				ptr += PM_SIZE;
// 				Data->IoStatus.Information -= PM_SIZE;
// 			}
			RtlCopyMemory(orgBuf, ptr, Data->IoStatus.Information);
		}
	}

	logi((":            %wZ newB=%p info=%Iu Freeing\n",
		&p2pCtx->Context->Name, p2pCtx->SwapBuffer, Data->IoStatus.Information));

	FltFreePoolAlignedWithTag(FltObjects->Instance, p2pCtx->SwapBuffer, BUFFER_SWAP_TAG);

	FltReleaseContext(p2pCtx->Context);

	ExFreeToNPagedLookasideList(&gPre2PostContexList, p2pCtx);

	return FLT_POSTOP_FINISHED_PROCESSING;
}