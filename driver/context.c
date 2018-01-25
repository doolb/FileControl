#include "minidriver.h"

//
// detech volume name and set in to context
//
NTSTATUS volumeDetech(_In_ PCFLT_RELATED_OBJECTS FltObjects){

	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT devobj = null;

	PVolumeContext ctx = NULL;


	ULONG retLen;
	USHORT size;
	//
	// get the volume name
	//
	try
	{
		//
		// allocate a volume context
		//
		status = FltAllocateContext(FltObjects->Filter, FLT_VOLUME_CONTEXT, sizeof(VolumeContext), NonPagedPool, &ctx);
		if (!NT_SUCCESS(status)){
			loge((NAME"create context failed. %x", status));
			leave;
		}

		//
		//  Always get the volume properties, so I can get a sector size
		//
		ctx->prop = (PFLT_VOLUME_PROPERTIES)ctx->_prop_buffer;
		status = FltGetVolumeProperties(FltObjects->Volume, ctx->prop, sizeof(ctx->_prop_buffer), &retLen);
		if (!NT_SUCCESS(status)){
			loge((NAME"get volume properties failed. %x", status));
			leave;
		}

		//
		// save volume sector size
		//
		FLT_ASSERT((ctx->prop->SectorSize == 0) || (ctx->prop->SectorSize >= MIN_SECTOR_SIZE));
		ctx->SectorSize = max(ctx->prop->SectorSize, MIN_SECTOR_SIZE);
		ctx->Name.Buffer = NULL;

#pragma region get volume name
		//
		//  Get the storage device object we want a name for.
		//
		status = FltGetDiskDeviceObject(FltObjects->Volume, &devobj);
		if (NT_SUCCESS(status)){
			//
			//  Try and get the DOS name.  If it succeeds we will have
			//  an allocated name buffer.  If not, it will be NULL
			//
			status = RtlVolumeDeviceToDosName(devobj, &ctx->Name);
		}

		//
		//  Figure out which name to use from the properties
		//
		if (ctx->prop->RealDeviceName.Length > 0) {
			ctx->WorkName = &ctx->prop->RealDeviceName;
		}
		else if (ctx->prop->FileSystemDeviceName.Length > 0) {
			ctx->WorkName = &ctx->prop->FileSystemDeviceName;
		}
		else {
			ctx->WorkName = NULL;
		}

		//
		//  If we could not get a DOS name, get the NT name.
		//
		if (!NT_SUCCESS(status)) {

			FLT_ASSERT(ctx->Name.Buffer == NULL);

			if (ctx->WorkName == NULL){
				//  No name, don't save the context
				status = STATUS_FLT_DO_NOT_ATTACH;
				leave;
			}

			//  Get size of buffer to allocate.  This is the length of the string plus room for a trailing colon.
			size = ctx->WorkName->Length + sizeof(WCHAR);

			//  Now allocate a buffer to hold this name
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "ctx->Name.Buffer will not be leaked because it is freed in CleanupVolumeContext")
			ctx->Name.Buffer = ExAllocatePoolWithTag(NonPagedPool,
				size,
				NAME_TAG);
			if (ctx->Name.Buffer == NULL) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}

			//  Init the rest of the fields
			ctx->Name.Length = 0;
			ctx->Name.MaximumLength = size;

			RtlCopyUnicodeString(&ctx->Name, ctx->WorkName);	//  Copy the name in
			RtlAppendUnicodeToString(&ctx->Name, L":");	//  Put a trailing colon to make the display look good
		}

#pragma endregion


#pragma region GUID
		//
		// get volume guid
		// we must call this function in instance-setup routine
		//
		ctx->GUID.Buffer = NULL;
		ctx->GUID.Length = 0;
		ctx->GUID.MaximumLength = 0;		// first get the name length required

		if (FlagOn(ctx->prop->DeviceCharacteristics, FILE_REMOVABLE_MEDIA) ||
			ctx->prop->DeviceCharacteristics == 0){
			status = FltGetVolumeGuidName(FltObjects->Volume, &ctx->GUID, &retLen);
			if (!NT_SUCCESS(status)){
				// the we allocate the buffer
				ctx->GUID.MaximumLength = (USHORT)++retLen;
				ctx->GUID.Length = 0;
				ctx->GUID.Buffer = ExAllocatePoolWithTag(NonPagedPool, retLen, NAME_TAG);
				if (ctx->GUID.Buffer == NULL) {
					// get buffer failed
					status = STATUS_INSUFFICIENT_RESOURCES;
					leave;
				}

				// try again
				status = FltGetVolumeGuidName(FltObjects->Volume, &ctx->GUID, &retLen);
				if (!NT_SUCCESS(status)) {
					// get guid failed
					leave;
				}
			}
		}
		else{
			// we dont need to attach this device
			status = STATUS_FLT_DO_NOT_ATTACH;
			leave;
		}
#pragma endregion


		//
		// setup volume context
		//
		status = FltSetVolumeContext(FltObjects->Volume, FLT_SET_CONTEXT_KEEP_IF_EXISTS, ctx, NULL);

		//
		//  Log debug info
		//
		logw((NAME"setup volume context: Name=\"%wZ\",Device:%02x.%08x, Guid:%wZ, SectSize=0x%04x, %wZ\n", &ctx->Name, ctx->prop->DeviceType, ctx->prop->DeviceCharacteristics, &ctx->GUID, ctx->SectorSize, ctx->WorkName));

		//
		//  It is OK for the context to already be defined.
		//
		if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) status = STATUS_SUCCESS;
	}
	finally{
		//
		//  Always release the context.  If the set failed, it will free the
		//  context.  If not, it will remove the reference added by the set.
		//  Note that the name buffer in the ctx will get freed by the context
		//  cleanup routine.
		//
		if (ctx) {
			FltReleaseContext(ctx);
		}
		//  Remove the reference added to the device object by FltGetDiskDeviceObject.
		if (devobj){ ObDereferenceObject(devobj); }
	}

	return status;
}


//
// free context buffer
//
VOID CleanupVolumeContext(_In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType){
	PVolumeContext ctx = Context;
	PAGED_CODE();

	//
	//  Log debug info
	//
	logw((NAME"clear volume context: Name=\"%wZ\", Device:%02x.%08x, Guid:%wZ, SectSize=0x%04x\n", &ctx->Name, ctx->prop->DeviceType, ctx->prop->DeviceCharacteristics, &ctx->GUID, ctx->SectorSize));

	//
	// clear buffer
	//
	if (ContextType == FLT_VOLUME_CONTEXT){
		if (ctx->Name.Buffer != NULL){
			ExFreePool(ctx->Name.Buffer);
			ctx->Name.Buffer = NULL;
		}

		if (ctx->GUID.Buffer){
			ExFreePool(ctx->GUID.Buffer);
			ctx->GUID.Buffer = NULL;
		}
	}

}