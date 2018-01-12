#include "minidriver.h"

#pragma region Object

ULONG gLogFlag = ERROR|WARNING;

//
// global filter handler
//
PFLT_FILTER gFilter;


//
// filter callbacks
//
const FLT_OPERATION_REGISTRATION gMiniCallbacks[] = {
	{ IRP_MJ_CREATE, 0, miniPreCreate, miniPostCreate },
	{ IRP_MJ_WRITE, 0, miniPreWrite, miniPostWrite },
	{ IRP_MJ_OPERATION_END }	// END
};
//
// filter registion
//
const FLT_REGISTRATION gMiniRegistration = {
	sizeof(FLT_REGISTRATION),		// size
	FLT_REGISTRATION_VERSION,		// version
	0,								// flag

	NULL,							// context
	gMiniCallbacks,						// callback

	miniDriverUnload,					//  MiniFilterUnload

	NULL,							//  InstanceSetup
	NULL,							//  InstanceQueryTeardown
	NULL,							//  InstanceTeardownStart
	NULL,							//  InstanceTeardownComplete

	NULL,                           //  GenerateFileName
	NULL,                           //  GenerateDestinationFileName
	NULL                            //  NormalizeNameComponent
};

#pragma endregion

#pragma region Function

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath){
	loge((NAME"%wZ", RegistryPath));

	NTSTATUS status;

	//
	// registry filter driver
	//
	status = FltRegisterFilter(DriverObject, &gMiniRegistration, &gFilter);
	if (NT_SUCCESS(status)){
		//
		// start filter i/o
		//
		status = FltStartFiltering(gFilter);
		if (!NT_SUCCESS(status)){
			FltUnregisterFilter(gFilter);
			KdPrint((NAME"%x", status));
			return status;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags){
	UNREFERENCED_PARAMETER(flags);

	loge((NAME"%x\n", flags));
	FltUnregisterFilter(gFilter);
	return STATUS_SUCCESS;
}
#pragma endregion
