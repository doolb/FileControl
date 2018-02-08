#include <guiddef.h>
typedef enum _PermissionCode{
	PC_Invalid  = 0,
	PC_User_Read = 0x00000001,
	PC_User_Write = 0x00000002,
	PC_Group_Read = 0x00000004,
	PC_Group_Write = 0x00000008,
	PC_Other_Read = 0x00000010,
	PC_Other_Write = 0x00000020,

	PC_Default = PC_User_Read | PC_User_Write | PC_Group_Read
}PermissionCode, *PPermissionCode;


#define PM_ALL_SIZE  44		// the size of all permission data 
#define PM_DATA_SIZE 40		// the size of all permission data without the checksum

//
// the permission data of file
//
typedef struct _Permission
{
	unsigned long  _head;	// 4 byte
	PermissionCode code;		// 4 byte
	GUID uid;				// 16 byte
	GUID gid;				// 16 byte
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
void freePermission(PCFLT_RELATED_OBJECTS _obj,PPermission pm);