#include "minidriver.h"

#pragma region Object

ULONG gLogFlag = ERROR|WARNING;

//
// global filter handler
//
PFLT_FILTER gFilter;
PFLT_PORT gPort;

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
	logw((NAME"%wZ", RegistryPath));

	NTSTATUS status;

	//
	// registry filter driver
	//
	status = FltRegisterFilter(DriverObject, &gMiniRegistration, &gFilter);
	if (NT_SUCCESS(status)){

		//
		// create a communication port for user application
		//
		PSECURITY_DESCRIPTOR sd;
		status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
		if (NT_SUCCESS(status)){
			OBJECT_ATTRIBUTES oa = null;
			UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\fc");
			InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);
			status = FltCreateCommunicationPort(gFilter, &gPort, &oa, NULL, miniConnect, miniDisconnect, miniMessage, 1);
			FltFreeSecurityDescriptor(sd);

			if (NT_SUCCESS(status)){
				//
				// start filter i/o
				//
				status = FltStartFiltering(gFilter);
				if (NT_SUCCESS(status)){ return status; }
				loge((NAME"start filter driver failed. %x", status));

				//
				// failed,close port
				//
				FltCloseCommunicationPort(gPort);
			}
		}
		loge((NAME"create communication port failed. %x", status));

		//
		// failed, unregistry driver
		//
		FltUnregisterFilter(gFilter);
	}
	loge((NAME"registry driver failed. %x", status));

	return status;
}

NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags){
	FltCloseCommunicationPort(gPort);
	FltUnregisterFilter(gFilter);
	logw((NAME"%x\n", flags));
	return STATUS_SUCCESS;
}
#pragma endregion
