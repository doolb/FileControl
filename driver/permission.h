#pragma once
#include <guiddef.h>
#include "util.h"

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


#define PM_NAME_MAX 32

typedef struct
{
	WCHAR user[PM_NAME_MAX];			// user name
	WCHAR group[PM_NAME_MAX];		// group name

	GUID uid;				// user id
	GUID gid;				// group id
}User, *PUser;

#define PM_DATA_SIZE ( sizeof(User) + sizeof(ULONG) + sizeof(PermissionCode)	)	// the size of all permission data without the checksum
#define PM_ALL_SIZE  ( PM_DATA_SIZE + sizeof(UINT32) + sizeof(ULONG))		// the size of all permission data (add crc32, size)

// set the user name
#define pmSetName(ptr,name)  memcpy_s(&((ptr)->user.user), PM_NAME_MAX, name, wcsnlen(name,PM_NAME_MAX)*sizeof(WCHAR))
// set the group name
#define pmSetGroup(ptr,name) memcpy_s(&((ptr)->user.group), PM_NAME_MAX, name,wcsnlen(name,PM_NAME_MAX)*sizeof(WCHAR))

// is the same user of file
#define pmIsUser(pm,usr) 		(memcmp(&((pm)->user.uid), &((usr)->uid), sizeof(GUID)) == 0)
#define pmIsGroup(pm,usr) 		(memcmp(&((pm)->user.gid), &((usr)->gid), sizeof(GUID)) == 0)


//
// the permission data of file
//
typedef struct _Permission
{
	ULONG  _head;			// 4 byte
	PermissionCode code;		// 4 byte
	User   user;
	UINT32 crc32;			// 4 byte
	ULONG  sizeOnDisk;		// the size of data on disk, only avaliable in runtime
}Permission, *PPermission;



//
// permission
//
NTSTATUS checkFltStatus(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj);
// check the file status, is it is a dir and is need be filter
NTSTATUS checkPermission(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj, BOOLEAN iswrite);

NTSTATUS getPermission(PCFLT_RELATED_OBJECTS _obj, PPermission *_pm);
NTSTATUS setPermission(PCFLT_RELATED_OBJECTS _obj, PPermission pm);
void freePermission(PCFLT_RELATED_OBJECTS _obj, PPermission pm);

#pragma region User Key define

typedef struct _UserKey{
	User	user;				// user identify data
	UINT8	passwd[HASH_SIZE];	// user password hash
	UINT32	crc;				// check sum
}UserKey, *PUserKey;

struct _IUserKey
{
	NTSTATUS(*getAllUser)();	// get all user can login
	NTSTATUS(*login)();			// login a user 
	NTSTATUS(*signup)();		// signup a user
};

extern struct _IUserKey IUserKey[1];
#pragma endregion