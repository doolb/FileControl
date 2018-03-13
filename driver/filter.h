#pragma once
#include "minidriver.h"
#include "permission.h"
#include "msg.h"

typedef struct _VolumeList
{
	//
	// list entry member
	//
	LIST_ENTRY list;

	/************************************************************************/
	/* our member                                                                     */
	/************************************************************************/
	//
	// volume guid
	//
	UNICODE_STRING GUID;
	WCHAR _GUID_Buffer[GUID_SIZE];

	//
	// instance
	//
	PFLT_INSTANCE instance;

	//
	// volume letter
	//
	WCHAR letter;

	//
	// use key install on this volume
	//
	UserKey	key;

	//
	// volume state: (N:normal, W:work root, U:has user, K:user login root, R:removealbe) 
	//
	WCHAR state;
}VolumeList, *PVolumeList;


#define vl_isWorkRoot(vl) ((vl)->state == L'W')
#define vl_setWorkRoot(vl) ((vl)->state = L'W')

#define vl_isKeyRoot(vl) ((vl)->state == L'K')
#define vl_setKeyRoot(vl) ((vl)->state = L'K')

#define vl_ishasUser(vl) ((vl)->state == L'U')
#define vl_sethasUser(vl) ((vl)->state = L'U')

#define vl_isNormal(vl) ((vl)->state == L'N')
#define vl_setNormal(vl) ((vl)->state = L'N')

#define vl_isRemove(vl) ((vl)->state == L'R')
#define vl_setRemove(vl) ((vl)->state = L'R')

#define FLT_TAG 'Fttg'

//
// user filter interface 
//
NTSTATUS oninit(PUNICODE_STRING _regPath);	// call when dirver start
void onexit();		// call when dirver unload
NTSTATUS onstart(PVolumeContext ctx, PFLT_INSTANCE instance); // call when setup filter on volume
void onstop(PVolumeContext ctx);		// call when stop filter on volme
NTSTATUS onfilter(PFLT_FILE_NAME_INFORMATION info, PUNICODE_STRING guid);// call when filter data on volme
NTSTATUS onmsg(MsgCode msg, PVOID buffer, ULONG size, PULONG retlen);	// call when user application message in
NTSTATUS sendMsg(MsgCode msg);	// send message to user application
