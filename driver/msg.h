#pragma once
#include "rsa.h"
/************************************************************************/
/* driver and application msg define                                    */
/************************************************************************/
typedef enum {
	MsgCode_Null, // null define , for daemon use

	// user
	MsgCode_User_Query,
	MsgCode_User_Login,
	MsgCode_User_Login_Get,
	MsgCode_User_Registry,
	MsgCode_User_Logout,
	MsgCode_User_Del,

	// volume
	MsgCode_Volume_Query,

	// file
	MsgCode_Permission_Get,
	MsgCode_Permission_Set,

	// work root
	MsgCode_WorkRoot_Get,
	MsgCode_WorkRoot_Set,

	// admin rsa key
	MsgCode_Admin_Init,
	MsgCode_Admin_Check,
	MsgCode_Admin_Exit,

	MsgCode_Max

}MsgCode, *PMsgCode;

#define PM_NAME_MAX 32


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
	WCHAR user[PM_NAME_MAX];			// user name
	WCHAR group[PM_NAME_MAX];		// group name

	GUID uid;				// user id
	GUID gid;				// group id
}User, *PUser;


typedef enum
{
	Admin_Valid = 1,		// admin is valid
	Admin_CanLogin = 2,	// admin can login
	Admin_Login = 4,
	Admin_Has_Pri = 8,
}AdminUserState, *PAdminUserState;

typedef struct
{
	//
	// admin user key data
	//
	User admin;
	struct public_key_class pub;
	struct public_key_class pri;

	struct public_key_class sys; // the key for current system

	AdminUserState state;
}AdminUser, *PAdminUser;

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
	PWCHAR	path;					// file path
	User		user;					// the user which whole the file
	PermissionCode pmCode;			// permission code
}Msg_File, *PMsg_File;
