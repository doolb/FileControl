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


/************************************************************************/
/* driver msg define                                                                     */
/************************************************************************/
#pragma region msg
typedef enum {
	MsgCode_Null, // null define , for daemon use

	// user
	MsgCode_User_Get,
	MsgCode_User_Login,
	MsgCode_User_Sign,
	MsgCode_User_Logout,

	// file
	MsgCode_Permission_Get,
	MsgCode_Permission_Set,

	// driver
	MsgCode_GetPause,
	MsgCode_SetPause,
}MsgCode, *PMsgCode;

#define PM_NAME_MAX 32

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

typedef struct _Msg
{
	MsgCode code;

	union
	{
		struct
		{
			struct{
				ULONG count;
				PUser users;
			}query;

			struct
			{
				WCHAR  name[PM_NAME_MAX];		// user name
				WCHAR  group[PM_NAME_MAX];		// group name
				WCHAR  password[PM_NAME_MAX];	// password
				WCHAR  volume[GUID_SIZE];		// volume guid
			}signup;
		}User;

		struct
		{
			PWCHAR path;					// file path
			PermissionCode pmCode;		// permission code
		}File;

		struct
		{
			BOOL * pause;		// store the value of pause
		}Driver;
	}Data;
}Msg, *PMsg;

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

	HRESULT(*send)(PMsg msg);
	HRESULT(*listen)(PMsgCode msg);
};

extern FC_API struct _IFc IFc[1];