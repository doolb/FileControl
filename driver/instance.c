#include "minidriver.h"

/************************************************************************/
/* this file is handle the instance steup or remove for minidriver                                                                     */
/************************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,miniInsSteup)
#pragma alloc_text(PAGE,miniInsQeuryTeardown)
#pragma alloc_text(PAGE,miniInsTeardownStart)
#pragma alloc_text(PAGE,miniInsTeardownComplete)
#endif


extern NTSTATUS volumeDetech(_In_ PCFLT_RELATED_OBJECTS FltObjects);

//
// save global instance
//
PFLT_INSTANCE gInstance;

NTSTATUS miniInsSteup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	// save instance
	gInstance = FltObjects->Instance;

	NTSTATUS status = volumeDetech(FltObjects);

	log((NAME"%p,%d,%x,%x \n", FltObjects, Flags, VolumeDeviceType, VolumeFilesystemType));

	return status;		// -attach
	//return STATUS_FLT_DO_NOT_ATTACH - do not attach
}

NTSTATUS	 miniInsQeuryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	log((NAME"%p,%d \n", FltObjects, Flags));
	return STATUS_SUCCESS;
}
VOID	 miniInsTeardownStart(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	log((NAME"%p,%d \n", FltObjects, Flags));

}
VOID	 miniInsTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	log((NAME"%p,%d \n", FltObjects, Flags));
}

