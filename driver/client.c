#include "minidriver.h"
#include "filter.h"

PFLT_PORT gClient;
extern PFLT_FILTER gFilter;

NTSTATUS miniMessage(_In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength){
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	NTSTATUS status = STATUS_SUCCESS;

	//
	// check parameter
	//
	if (!InputBuffer || InputBufferLength != sizeof(MsgCode) ||
		!OutputBuffer || !OutputBufferLength ||
		!ReturnOutputBufferLength){
		loge((NAME"invalid message"));
		return STATUS_INVALID_PARAMETER;
	}

	//
	// msg
	//
	status = onmsg((*(PMsgCode)InputBuffer), OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);

	logf(LDBG, (NAME"client message in. (%x) %d[%d] %x[%d,%x] \n", PortCookie, (*(PMsgCode)InputBuffer), InputBufferLength, OutputBuffer, OutputBufferLength, *ReturnOutputBufferLength));
	return status;
}

NTSTATUS miniConnect(_In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionPortCookie){
	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	// normal port
	ASSERT(gClient == NULL);
	gClient = ClientPort;

	logw((NAME"client connect. %x \n", ServerPortCookie));
	return STATUS_SUCCESS;
}

//
// ConnectionCookie is the value of *ServerPortCookie
//
VOID miniDisconnect(_In_opt_ PVOID ConnectionCookie){
	FltCloseClientPort(gFilter, &gClient);
	logw((NAME"client disconnect. %x \n", ConnectionCookie));
}