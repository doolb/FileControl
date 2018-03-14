#include "minidriver.h"
#include "op.h"
#include "filter.h"

#pragma region Object

ULONG gLogFlag = ERROR | WARNING | LOG | INFO;

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
	{ IRP_MJ_QUERY_INFORMATION, 0, miniPreQueryInfo, miniPostQueryInfo },
	{ IRP_MJ_DIRECTORY_CONTROL, 0, miniPreDirCtrl, miniPostDirCtrl },
	{ IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, 0, miniPreAcqSection, NULL },
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

void stop(){

	//
	// notify exit
	//
	onexit();

	//
	// delete lookaside list
	//
	ExDeleteNPagedLookasideList(&gPre2PostContexList);

	//
	// close port
	//
	if (gPort) FltCloseCommunicationPort(gPort); gPort = NULL;

	//
	// unregistry driver
	//
	if (gFilter) FltUnregisterFilter(gFilter); gFilter = NULL;

	logw((NAME"driver stoped\n"));
}


#pragma code_seg("INIT")
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath){
	logw((NAME"%wZ \n", RegistryPath));

	NTSTATUS status = STATUS_SUCCESS;

	try{
		//
		// registry filter driver
		//
		status = FltRegisterFilter(DriverObject, &gMiniRegistration, &gFilter);
		if (!NT_SUCCESS(status)) { loge((NAME"registry driver failed. %x \n", status)); leave; }

		//
		// create a communication port for user application
		//
		PSECURITY_DESCRIPTOR sd;
		status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
		if (!NT_SUCCESS(status)) { loge((NAME"FltBuildDefaultSecurityDescriptor failed. %x \n", status)); leave; }

		OBJECT_ATTRIBUTES oa = null;
		//
		// normal port
		//
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\fc");
		InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);
		status = FltCreateCommunicationPort(gFilter, &gPort, &oa, NULL, miniConnect, miniDisconnect, miniMessage, 1);
		if (!NT_SUCCESS(status)) { loge((NAME"create communication port  failed. %x \n", status)); leave; }

		FltFreeSecurityDescriptor(sd);


		// init lookaside list
		ExInitializeNPagedLookasideList(&gPre2PostContexList, NULL, NULL, 0, sizeof(Pre2PostContext), PRE_2_POST_TAG, 0);

		//
		// call user inteface
		//
		status = oninit(RegistryPath);
		if (!NT_SUCCESS(status)) { loge((NAME"oninit failed. %x \n", status)); leave; }


		//
		// start filter i/o
		//
		status = FltStartFiltering(gFilter);
		if (!NT_SUCCESS(status)) { loge((NAME"start filter driver failed. %x \n", status)); leave; }
	}
	finally{
		if (!NT_SUCCESS(status)){

			stop();
		}
	}


	return status;
}
#pragma code_seg("PAGE")
NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags){
	PAGED_CODE();

	stop();

	logw((NAME"%x\n \n", flags));
	return STATUS_SUCCESS;
}
#pragma endregion
