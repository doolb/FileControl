#include "minidriver.h"
#include "msg.h"
/************************************************************************/
/* IRP_MJ_PNP
	use to detech usb remove message
	*/
/************************************************************************/
extern NTSTATUS sendMsg(MsgCode msg);

FLT_PREOP_CALLBACK_STATUS miniPrePnp(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext){
	UNREFERENCED_PARAMETER(_data);
	UNREFERENCED_PARAMETER(_fltObjects);
	UNREFERENCED_PARAMETER(_completionContext);
	logfi;
	
	//
	// is use remove usb surprise
	//
	if (_data->Iopb->MinorFunction == IRP_MN_SURPRISE_REMOVAL){

		NTSTATUS status = STATUS_SUCCESS;
		PVolumeContext ctx = NULL;

		try{
			//
			// get context
			//
			status = FltGetVolumeContext(_fltObjects->Filter, _fltObjects->Volume, &ctx);
			if (!NT_SUCCESS(status)){
				loge((NAME"get context failed. %x", status));
				leave;
			}
			logw((NAME"User remove usb surprise : %wZ, %wZ.", &ctx->Name, ctx->WorkName));
		}
		finally{
			if (ctx) FltReleaseContext(ctx);
			FltDeleteVolumeContext(_fltObjects->Filter, _fltObjects->Volume, NULL);
		}
	}

	logfo;
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
