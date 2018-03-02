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
typedef enum {
	MsgCode_Null, // null define , for daemon use

	// user
	MsgCode_User_Query,
	MsgCode_User_Login,
	MsgCode_User_Registry,
	MsgCode_User_Logout,

	// volume
	MsgCode_Volume_Query,

	// file
	MsgCode_Permission_Get,
	MsgCode_Permission_Set,

	// driver
	MsgCode_GetPause,
	MsgCode_SetPause,
}MsgCode, *PMsgCode;

#define PM_NAME_MAX 32
#define setWchar(ptr,name,max)	memcpy_s((ptr),max * sizeof(WCHAR), name, wcsnlen(name,max) * sizeof(WCHAR))
#define GUID_SIZE	64 // the length for volume guid string

typedef struct
{
	WCHAR user[PM_NAME_MAX];			// user name
	WCHAR group[PM_NAME_MAX];		// group name

	GUID uid;				// user id
	GUID gid;				// group id
}User, *PUser;

typedef enum _PermissionCode{
	PC_Invalid = 0,
	PC_User_Read = 0x00000001,
	PC_User_Write = 0x00000002,
	PC_Group_Read = 0x00000004,
	PC_Group_Write = 0x00000008,
	PC_Other_Read = 0x00000010,
	PC_Other_Write = 0x00000020,

	PC_Default = PC_User_Read | PC_User_Write | PC_Group_Read
}PermissionCode, *PPermissionCode;
typedef struct
{
	User	 user;
	WCHAR  password[PM_NAME_MAX];	// password
	WCHAR  letter;					// volume letter
}Msg_User_Registry, *PMsg_User_Registry;

typedef struct{
	User		user;					// user
	WCHAR	password[PM_NAME_MAX];	// password
}Msg_User_Login, *PMsg_User_Login;

typedef struct
{
	PWCHAR path;					// file path
	PermissionCode pmCode;		// permission code
}Msg_File, *PMsg_File;


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
