#pragma once
#include <guiddef.h>
#include "util.h"
#include "msg.h"
#include "rsa.h"

#define PM_TAG 'Pmtg'



#define PM_DATA_SIZE ( sizeof(User) + sizeof(ULONG) + sizeof(PermissionCode)	)	// the size of all permission data without the checksum
#define PM_SIZE 256

// set the user name
#define pmSetName(ptr,name)  memcpy_s(&((ptr)->user.user), PM_NAME_MAX*sizeof(WCHAR), name, wcsnlen(name,PM_NAME_MAX)*sizeof(WCHAR))
// set the group name
#define pmSetGroup(ptr,name) memcpy_s(&((ptr)->user.group), PM_NAME_MAX*sizeof(WCHAR), name,wcsnlen(name,PM_NAME_MAX)*sizeof(WCHAR))

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
}Permission, *PPermission;



//
// permission
//
NTSTATUS checkFltStatus(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj);
// check the file status, is it is a dir and is need be filter
NTSTATUS checkPermission(PFLT_CALLBACK_DATA _data, PCFLT_RELATED_OBJECTS _obj, BOOLEAN iswrite);
NTSTATUS cmpPermission(PPermission pm, BOOL iswrite);
NTSTATUS getPermission(PFLT_INSTANCE ins, PFILE_OBJECT obj, PPermission *_pm);
NTSTATUS setPermission(PFLT_INSTANCE ins, PFILE_OBJECT obj, PPermission pm);
void freePermission(PPermission pm);

#pragma region User Key define

#define USER_KEY_FILE	L"\\user.key"

typedef struct _UserKey{
	User		user;				// user identify data
	UINT8	passwd[HASH_SIZE];	// user password hash
	UINT32	crc;					// check sum
	UINT8   padding[12];			// padding for aes
}UserKey, *PUserKey;

#define ADMIN_KEY_FILE	L"\\admin.key"

typedef struct _AdminKey{
	User		user;				// user identify data
	UINT8	passwd[HASH_SIZE];	// user password hash
	struct public_key_class pub;// admin pub key
	struct public_key_class pri;// admin pri key
	UINT32	crc;					// check sum
	ULONG	valid;
	UINT8   padding[8];			// padding for aes
}AdminKey, *PAdminKey;

struct _IUserKey
{
	// read user from file
	NTSTATUS(*read)(PFLT_INSTANCE instance, PUNICODE_STRING path, PUserKey key);
	// write a user data to file
	NTSTATUS(*write)(PFLT_INSTANCE instance, PUNICODE_STRING path, PUserKey key);
	// registry a user
	PUserKey(*registry)(PUNICODE_STRING path, PVOID reg);
	// delete user
	NTSTATUS(*delete)(PFLT_INSTANCE instance, PUNICODE_STRING path);
};

struct _IAdminKey
{
	// read user from file
	NTSTATUS(*read)(PFLT_INSTANCE instance, PUNICODE_STRING path, PAdminKey key);
	// write a user data to file
	NTSTATUS(*write)(PFLT_INSTANCE instance, PUNICODE_STRING path, PAdminKey key);
	// registry a user
	NTSTATUS(*registry)(PUNICODE_STRING path, PMsg_Admin_Registry reg, PAdminKey key);
	// delete user
	NTSTATUS(*delete)(PFLT_INSTANCE instance, PUNICODE_STRING path);
};
extern struct _IUserKey IUserKey[1];
extern struct _IAdminKey IAdminKey[1];

#pragma endregion