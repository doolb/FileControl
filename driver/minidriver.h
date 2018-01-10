#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#define TAG "MNFL"
#define NAME "[Mini Filter]:"

//
// filter driver entry point
//
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
//
// mini driver unload
//
NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags);

#pragma region Mini Driver IO

#pragma endregion
