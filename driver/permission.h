#include <guiddef.h>
typedef enum _PermissionCode{
	PC_User_Read = 0x00000001,
	PC_User_Write = 0x00000002,
	PC_Group_Read = 0x00000004,
	PC_Group_Write = 0x00000008,
	PC_Other_Read = 0x00000010,
	PC_Other_Write = 0x00000020,

	PC_Default = PC_User_Read | PC_User_Write | PC_Group_Read
}PermissionCode, *PPermissionCode;


#define HEAD_LENGTH 60
#define PADDING 20
//
// нд╪Ч
//
typedef struct _Permission
{
	unsigned long  _head;
	PermissionCode code;
	GUID uid;
	GUID gid;
	char  _pad[PADDING]; // padding  (size 64)
	UINT32 crc32;
}Permission, *PPermission;