#include "minidriver.h"
#include "filter.h"

static PFLT_PORT gClient;
PFLT_PORT gDaemonClient;
extern PFLT_FILTER gFilter;

NTSTATUS miniMessage(_In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength){
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	ASSERT(InputBuffer && OutputBuffer);
	ASSERT(InputBufferLength == sizeof(Msg));
	ASSERT(OutputBufferLength == sizeof(Msg));

	NTSTATUS status = STATUS_SUCCESS;

	if (PortCookie == NULL){
		// normal port
		status = onmsg((PMsg)InputBuffer);
		RtlCopyMemory(OutputBuffer, InputBuffer, sizeof(Msg));
		*ReturnOutputBufferLength = sizeof(Msg);
	}
	else if (*((PULONG)PortCookie) == DAEMON_COOKIE){
		// daemon port
		status = STATUS_PENDING;
	}

	log((NAME"client message in. (%x) %ws[%d] %x[%d,%x] \n", PortCookie, (PWCH)InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, *ReturnOutputBufferLength));
	return status;
}

NTSTATUS miniConnect(_In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionPortCookie){
	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	if (ServerPortCookie == NULL){
		// normal port
		ASSERT(gClient == NULL);
		gClient = ClientPort;
	}
	else if (*((PULONG)ServerPortCookie) == DAEMON_COOKIE){
		// daemon port
		ASSERT(gDaemonClient == NULL);
		ASSERT(ConnectionPortCookie);

		gDaemonClient = ClientPort;
		*((PULONG)ConnectionPortCookie) = *((PULONG)ServerPortCookie);
	}

	logw((NAME"client connect. %x \n", ServerPortCookie));
	return STATUS_SUCCESS;
}

//
// ConnectionCookie is the value of *ServerPortCookie
//
VOID miniDisconnect(_In_opt_ PVOID ConnectionCookie){
	if (ConnectionCookie == 0){
		// normal port
		FltCloseClientPort(gFilter, &gClient);
	}
	else if (((ULONG)ConnectionCookie) == DAEMON_COOKIE){
		// daemon port
		FltCloseClientPort(gFilter, &gDaemonClient);
	}
	logw((NAME"client disconnect. %x \n", ConnectionCookie));
}