#pragma once
/************************************************************************/
/* driver and application msg define                                                                     */
/************************************************************************/
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
