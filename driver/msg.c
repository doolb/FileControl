#include <fltKernel.h>
#include "msg.h"
#include "filter.h"

extern NTSTATUS user_query(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS user_login(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS user_login_get(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS user_registry(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS user_logout(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS user_delete(void* buffer, unsigned long size, unsigned long *retlen);

extern NTSTATUS file_get(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS file_set(void* buffer, unsigned long size, unsigned long *retlen);

extern NTSTATUS workroot_get(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS workroot_set(void* buffer, unsigned long size, unsigned long *retlen);

extern NTSTATUS msg_query(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS volume_query(void* buffer, unsigned long size, unsigned long *retlen);

extern NTSTATUS admin_new(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS admin_check(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS admin_exit(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS admin_query(void* buffer, unsigned long size, unsigned long *retlen);
extern NTSTATUS admin_set(void* buffer, unsigned long size, unsigned long *retlen);

NTSTATUS(*MsgHandle[MsgCode_Max + 1])(void* buffer, unsigned long size, unsigned long *retlen) = {
	msg_query,	//MsgCode_Null,

	// user
	user_query,		//MsgCode_User_Query,
	user_login,		//MsgCode_User_Login,
	user_login_get,	//MsgCode_User_Login_Get
	user_registry,	//MsgCode_User_Registry,
	user_logout,		//MsgCode_User_Logout,
	user_delete,		//MsgCode_User_Del,

	// volume
	volume_query,	//MsgCode_Volume_Query,

	// file
	file_get,	//MsgCode_Permission_Get,
	file_set,	//MsgCode_Permission_Set,

	// work root
	workroot_get,	//MsgCode_WorkRoot_Get,
	workroot_set,	//MsgCode_WorkRoot_Set,

	admin_new,		//MsgCode_Admin_Init,
	admin_query,		//MsgCode_Admin_Qeury,
	admin_set,		//MsgCode_Admin_Set
};

// NTSTATUS func(void* buffer, unsigned long size, unsigned long *retlen){
// 
// 	NTSTATUS status = STATUS_SUCCESS;
// 	PLIST_ENTRY head = &gVolumeList;
// 	PVolumeList list = NULL;
// 
// }
