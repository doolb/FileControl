#include "minidriver.h"

#pragma region Object

ULONG gLogFlag = ERROR | WARNING;

PFLT_FILTER gFilter;		// global filter handler
PFLT_PORT	gPort;		// comminication port created for client login

NPAGED_LOOKASIDE_LIST gPre2PostContexList;	//  This is a lookAside list used to allocate our pre-2-post structure.

//
// filter callbacks
//
const FLT_OPERATION_REGISTRATION gMiniCallbacks[] = {
	{ IRP_MJ_CREATE, 0, miniPreCreate, miniPostCreate },
	{ IRP_MJ_WRITE, 0, miniPreWrite, miniPostWrite },
	{ IRP_MJ_READ, 0, miniPreRead, miniPostRead },
	{ IRP_MJ_PNP, 0, miniPrePnp, NULL },
	{ IRP_MJ_OPERATION_END }	// END
};

//
// Context definitions 
//
const FLT_CONTEXT_REGISTRATION gMiniContexts[] = {
	{
		FLT_VOLUME_CONTEXT,
		0,
		CleanupVolumeContext,
		sizeof(VolumeContext),
		CONTEXT_TAG
	},

	{ FLT_CONTEXT_END }
};

//
// filter registion
//
const FLT_REGISTRATION gMiniRegistration = {
	sizeof(FLT_REGISTRATION),		// size
	FLT_REGISTRATION_VERSION,		// version
	0,								// flag

	gMiniContexts,						// context
	gMiniCallbacks,						// callback

	miniDriverUnload,					//  MiniFilterUnload

	miniInsSteup,							//  InstanceSetup
	miniInsQeuryTeardown,					//  InstanceQueryTeardown
	miniInsTeardownStart,					//  InstanceTeardownStart
	miniInsTeardownComplete,					//  InstanceTeardownComplete

	NULL,                           //  GenerateFileName
	NULL,                           //  GenerateDestinationFileName
	NULL                            //  NormalizeNameComponent
};

#pragma endregion

#pragma region Function
#pragma code_seg("INIT")
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
#pragma code_seg("PAGE")
NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags){
	PAGED_CODE();

	FltCloseCommunicationPort(gPort);
	FltUnregisterFilter(gFilter);
	logw((NAME"%x\n", flags));
	return STATUS_SUCCESS;
}
#pragma endregion
