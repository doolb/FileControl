#pragma once
#define UNICODE
#include <windows.h>
#include <fltUser.h>
#ifndef bool
#define true 1
#define false 0
#define bool long
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

#if _DEBUG
#define log printf
#else
#define log 
#endif

//
// open coomunication with driver
//
FC_API bool fc_open();
//
// close coomunication with driver
//
FC_API void fc_close();
//
// send message to driver (sync)
// return the bytes count of valid out buffer
FC_API int fc_send(PWCH msg, int len, PWCH out, int outLen);
typedef void(*fc_on_msg)(WCHAR _in, int _inLen, WCHAR _out, int _outLen);
