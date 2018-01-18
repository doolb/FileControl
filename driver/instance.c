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

NTSTATUS miniInsSteup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	PAGED_CODE();

	logw((NAME"%wZ,%d,%x,%x",FltObjects->Volume,Flags,VolumeDeviceType,VolumeFilesystemType));
	return STATUS_SUCCESS;		// -attach
	//return STATUS_FLT_DO_NOT_ATTACH - do not attach
}

NTSTATUS	 miniInsQeuryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	
	PAGED_CODE();

	logw((NAME"%p,%d", FltObjects, Flags));
	return STATUS_SUCCESS;
}
VOID	 miniInsTeardownStart(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	logw((NAME"%p,%d", FltObjects, Flags));

}
VOID	 miniInsTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags){
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	logw((NAME"%p,%d", FltObjects, Flags));
}