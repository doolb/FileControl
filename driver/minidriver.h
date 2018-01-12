#pragma once
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>


#define LOG 0x001
#define WARNING 0x010
#define ERROR 0x100
extern ULONG gLogFlag;
#define log(_x_) (FlagOn(gLogFlag,(LOG)) ? DbgPrint _x_  : ((int)(0)))
#define logw(_x_) (FlagOn(gLogFlag,(WARNING)) ? DbgPrint _x_  : ((int)(0)))
#define loge(_x_) (FlagOn(gLogFlag,(ERROR)) ? DbgPrint _x_  : ((int)(0)))
#define logf(flag,_x_) (FlagOn(gLogFlag,(flag)) ? DbgPrint _x_  : ((int)(0)))

#define TAG "MNFL"
#define NAME "[Mini Filter]@"__FUNCTION__": "


//
// filter driver entry point
//
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
//
// mini driver unload
//
NTSTATUS miniDriverUnload(_In_ FLT_FILTER_UNLOAD_FLAGS flags);

#pragma region Mini Driver IO
//
// create
//
FLT_PREOP_CALLBACK_STATUS miniPreCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext);
FLT_POSTOP_CALLBACK_STATUS miniPostCreate(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags);
//
// write
//
FLT_PREOP_CALLBACK_STATUS miniPreWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext);
FLT_POSTOP_CALLBACK_STATUS miniPostWrite(_Inout_ PFLT_CALLBACK_DATA _data, _In_ PCFLT_RELATED_OBJECTS _fltObjects, _In_opt_ PVOID *_completionContext, _In_ FLT_POST_OPERATION_FLAGS _flags);

#pragma endregion
