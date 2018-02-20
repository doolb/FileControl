#pragma once
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <minwindef.h>

#define null { 0 }

#define LOG 0x001
#define WARNING 0x010
#define ERROR 0x100
#define INFO 0x1000

#define BUFFER_SWAP_TAG     'bfBS'
#define CONTEXT_TAG         'xaBS'
#define NAME_TAG            'mbBS'
#define PRE_2_POST_TAG      'paBS'

#define logd KdPrint	
extern ULONG gLogFlag;

#define log(_x_) (FlagOn(gLogFlag,(LOG)) ? DbgPrint _x_  : ((int)(0)))
#define logw(_x_) (FlagOn(gLogFlag,(WARNING)) ? DbgPrint _x_  : ((int)(0)))
#define loge(_x_) (FlagOn(gLogFlag,(ERROR)) ? DbgPrint _x_  : ((int)(0)))
#define logf(flag,_x_) (FlagOn(gLogFlag,(flag)) ? DbgPrint _x_  : ((int)(0)))
#define logi(_x_) logf(INFO,_x_)

#define TAG "MNFL"
#define NAME "[Mini Filter]@"__FUNCTION__": "

// file no need to filter
#define FLT_NO_NEED ((NTSTATUS)0x01000001L)
#define FLT_ON_DIR  ((NTSTATUS)0x01000002L)
#define FLT_NEED		((NTSTATUS)0x01000003L)

//
// context registion
//
typedef struct _VolumeContext
{
	UNICODE_STRING	Name;				// volume name
	ULONG			SectorSize;			// the sector size for this volume.
	PUNICODE_STRING WorkName;			// work name for device (this will be FileSystemDeviceName , RealDeviceName or null)
	UNICODE_STRING GUID;					// volume GUID

	//
	// save the volume context so you can get more information
	//
	PFLT_VOLUME_PROPERTIES prop;
	UCHAR _prop_buffer[sizeof(FLT_VOLUME_PROPERTIES) + 512]; // volume property buffer

	//
	// permission head size
	//
	ULONG PmHeadSize;
}VolumeContext, *PVolumeContext;
#define MIN_SECTOR_SIZE 0x200
#define DEFAULT_SECTOR_SIZE 4096

//
// context struct pass to post callback from pre callback
//
typedef struct _Pre2PostContext
{
	PVolumeContext	Context;		// volume context alloc from pre callback
	PVOID			SwapBuffer;	// the new buffer for file data
}Pre2PostContext, *PPre2PostContext;


//
// filter driver entry point
//
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
//
// mini driver unload
//
NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags);



//
// volume context cleanup
//
VOID CleanupVolumeContext(_In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType);

//
// client message
//
NTSTATUS miniMessage(_In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength);
NTSTATUS miniConnect(_In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionPortCookie);
VOID miniDisconnect(_In_opt_ PVOID ConnectionCookie);

//
// instance
//
NTSTATUS miniInsSteup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);
NTSTATUS	 miniInsQeuryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);
VOID	 miniInsTeardownStart(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);
VOID	 miniInsTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);


//
// user filter interface 
//
NTSTATUS oninit(PUNICODE_STRING _regPath);	// call when dirver start
void onexit();		// call when dirver unload
NTSTATUS onstart(PVolumeContext ctx); // call when setup filter on volume
void onstop(PVolumeContext ctx);		// call when stop filter on volme
NTSTATUS onfilter(PFLT_FILE_NAME_INFORMATION info, PUNICODE_STRING guid);// call when filter data on volme
NTSTATUS onmsg();	// call when user application message in

#pragma endregion
