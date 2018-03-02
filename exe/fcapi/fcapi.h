#pragma once
#define UNICODE
#include <windows.h>
#include <fltUser.h>
#include "msg.h"

#ifndef bool
typedef enum _bool
{
	false = 0,
	true = 1
}bool;
#endif


#ifdef  FC_STATIC	// is use static library
#define FC_API 
#else
#ifdef  FC_IMPLEMENT
#define FC_API __declspec(dllexport)
#else
#define FC_API __declspec(dllimport)
#endif
#endif

#define FC_PORT_NAME L"\\fc"
#define FC_DAEMON_PORT_NAME L"\\fc-d"

#if _DEBUG
#define log printf
#else
#define log 
#endif

#define null		{0}

//
// dirver return code
//
#define E_INVALID_PASSWORD		((NTSTATUS)0xC1000004L)
#define E_NO_USER				((NTSTATUS)0xC1000005L)

/************************************************************************/
/* driver msg define                                                                     */
/************************************************************************/
#pragma region msg
#define setWchar(ptr,name,max)	memcpy_s((ptr),max * sizeof(WCHAR), name, wcsnlen(name,max) * sizeof(WCHAR))
#define GUID_SIZE	64 // the length for volume guid string

typedef struct _MsgData
{
	FILTER_MESSAGE_HEADER _head;
	MsgCode code;
}MsgData, *PMsgData;
#pragma endregion

struct _IFc
{
	bool(*open)();
	void(*close)();

	HRESULT(*send)(MsgCode msg, PVOID buffer, ULONG size, PULONG retlen);
	HRESULT(*listen)(PMsgCode msg);

	int(*queryUser)(PUser *users);
};

extern FC_API struct _IFc IFc[1];

#pragma region dll export function 
FC_API bool fc_open(bool isdaemon);
FC_API void fc_close();
FC_API HRESULT fc_send(MsgCode msg, PVOID buffer, ULONG size, PULONG retlen);
FC_API HRESULT fc_listen(PMsgCode code);
FC_API int fc_query_user(PUser *users);
#pragma endregion
