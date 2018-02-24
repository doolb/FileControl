#pragma once
#define UNICODE
#include <windows.h>
#include <fltUser.h>
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

//
// open coomunication with driver
//
FC_API bool fc_open(bool isdaemon);
//
// close coomunication with driver
//
FC_API void fc_close(bool isdaemon);
//
// send message to driver (sync)
// return the bytes count of valid out buffer
FC_API int fc_send(bool isdaemon, PWCH msg, int len, PWCH out, int outLen);

