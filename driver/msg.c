#include <fltKernel.h>
#include "msg.h"
#include "filter.h"

extern UNICODE_STRING gWorkRoot;
extern UNICODE_STRING gKeyRoot;
extern PFLT_INSTANCE gInstance;
extern LIST_ENTRY gVolumeList;
extern WCHAR gWorkRootLetter;
extern PFLT_FILTER gFilter;
extern MsgCode currentMsg;
extern HANDLE gRegistry;
extern User gUser;

NTSTATUS user_query(void* buffer, unsigned long size, unsigned long *retlen){

	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;

	int count = 0;
	//
	// get the all user count
	//	
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (vl_ishasUser(list))
			count++;
	}
	*retlen = count * sizeof(User);

	//
	// check the buffer and size
	//
	if (count == 0){
		status = STATUS_SUCCESS;
	}
	else if (buffer && size >= count * sizeof(User)){
		ULONG buff = (ULONG)buffer;

		for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
			list = CONTAINING_RECORD(e, VolumeList, list);
			if (vl_ishasUser(list)){
				memcpy_s((PVOID)buff, sizeof(User), &list->key.user, sizeof(User));

				buff += sizeof(User);
			}
		}
	}
	else{
		status = STATUS_BUFFER_TOO_SMALL;
	}

	return status;
}

NTSTATUS user_registry(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;

	if (!buffer || size < sizeof(Msg_User_Registry)){ *retlen = sizeof(Msg_User_Registry); return STATUS_BUFFER_TOO_SMALL; }

	PMsg_User_Registry reg = buffer;
	PVolumeList volume = NULL;
	//
	// found the volume
	//
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (list->letter == reg->letter){
			volume = list;
			break;
		}
	}
	if (!volume){ return FLT_NO_USER; }

	//
	// registry user
	//
	PUserKey key = IUserKey->registry(&volume->GUID, reg);
	if (!key){ loge((NAME"registry user failed. %ws", reg->letter)); return STATUS_FAIL_FAST_EXCEPTION; }

	//
	// write user key
	//
	status = IUserKey->write(list->instance, &volume->GUID, key);
	if (!NT_SUCCESS(status)){ loge((NAME"write user key failed. %ws", reg->letter)); return STATUS_FAIL_FAST_EXCEPTION; }

	//
	// save key to volume lsit
	//
	memcpy_s(&volume->key, sizeof(UserKey), key, sizeof(UserKey));
	vl_sethasUser(volume);

	return status;
}

NTSTATUS user_login(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;
	if (!buffer || size < sizeof(Msg_User_Login)){ *retlen = sizeof(Msg_User_Login); return STATUS_BUFFER_TOO_SMALL; }

	//
	// is already login
	//
	if (gKeyRoot.Length != 0){ return STATUS_SUCCESS; }

	PMsg_User_Login login = buffer;
	PVolumeList volume = NULL;
	//
	// found the volume
	//
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (vl_ishasUser(list) &&
			memcmp(&list->key.user.uid, &login->user.uid, sizeof(GUID)) == 0){
			volume = list;
			break;
		}
	}
	if (!volume){ return FLT_NO_USER; }

	//
	// compare password
	//
	UINT8 hash[HASH_SIZE];
	IUtil->hash((PUINT8)login->password, wcsnlen(login->password, PM_NAME_MAX) * sizeof(WCHAR), hash);
	if (memcmp(hash, volume->key.passwd, HASH_SIZE) != 0){ loge((NAME"uncorrect password.\n")); return FLT_INVALID_PASSWORD; }

	//
	// login user
	//
	RtlCopyUnicodeString(&gKeyRoot, &volume->GUID);
	memcpy_s(&gUser, sizeof(User), &volume->key.user, sizeof(User));
	vl_setKeyRoot(volume);

	loge((NAME"user login in. %ws", volume->letter));
	status = STATUS_SUCCESS;
	return status;
}
NTSTATUS user_delete(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;
	if (!buffer || size < sizeof(WCHAR)){ *retlen = sizeof(WCHAR); return STATUS_BUFFER_TOO_SMALL; }

	PVolumeList volume = NULL;
	WCHAR letter = *(PWCHAR)(buffer);
	//
	// found the volume
	//
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (vl_ishasUser(list) &&
			list->letter == letter){
			volume = list;
			break;
		}
	}
	if (!volume){ return FLT_NO_USER; }

	//
	// is already login
	//
	if (vl_isKeyRoot(volume)){ gKeyRoot.Length = 0; }
	vl_setRemove(volume); // set volume state

	//
	// delete file
	//
	IUserKey->delete(volume->instance, &volume->GUID);

	loge((NAME"user deleted. %ws", volume->letter));
	status = STATUS_SUCCESS;
	return status;
}

NTSTATUS user_login_get(void* buffer, unsigned long size, unsigned long *retlen){
	//
	// is login
	//
	if (gKeyRoot.Length == 0){ return STATUS_ACCESS_DENIED; }

	//
	// check buffer
	//
	if (!buffer || size < sizeof(User)){ *retlen = sizeof(User); return STATUS_BUFFER_TOO_SMALL; }

	memcpy_s(buffer, sizeof(User), &gUser, sizeof(User));
	*retlen = sizeof(User);
	return STATUS_SUCCESS;
}

NTSTATUS user_logout(void* buffer, unsigned long size, unsigned long *retlen){
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(retlen);


	gKeyRoot.Length = 0;
	return STATUS_SUCCESS;
}

NTSTATUS file_get(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;

	//
	// check buffer and size
	//
	if (!buffer || size < sizeof(Msg_File)){ *retlen = sizeof(Msg_File); return STATUS_BUFFER_TOO_SMALL; }

	//
	// has work root
	//
	if (!gInstance) { loge((NAME"no work root \n")); return STATUS_ACCESS_DENIED; }

	//
	// check parameter
	//
	PMsg_File file = buffer;
	if (file->path == NULL){ loge((NAME"invalid file path \n")); return STATUS_INVALID_PARAMETER; }

	//
	// check file path
	//
	if (!wcsstr(file->path, gWorkRoot.Buffer)){ loge((NAME"file isnot in work root \n")); return STATUS_INVALID_PARAMETER; }

	//
	// open file
	//
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, file->path);
	OBJECT_ATTRIBUTES oa = null;
	InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	PFILE_OBJECT obj = NULL;
	HANDLE hand = NULL;
	IO_STATUS_BLOCK iostatus;
	status = FltCreateFileEx(gFilter, gInstance, &hand, &obj, FILE_GENERIC_READ, &oa, &iostatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
	if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); return status; }

	//
	// get permission
	//
	PPermission pm = NULL;
	status = getPermission(gInstance, obj, &pm);
	if (!NT_SUCCESS(status)){ loge((NAME"getPermission failed. %x.%wZ", status, &name)); FltClose(hand); return status; }

	//
	// copy data
	//
	file->pmCode = pm->code;
	memcpy_s(&file->user, sizeof(User), &pm->user, sizeof(User));
	*retlen = sizeof(Msg_File);

	//
	// clear
	//
	freePermission(pm);
	FltClose(hand);
	return status;

}

NTSTATUS file_set(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;

	//
	// check buffer and size
	//
	if (!buffer || size < sizeof(Msg_File)){ *retlen = sizeof(Msg_File); return STATUS_BUFFER_TOO_SMALL; }

	//
	// has work root
	//
	if (!gInstance) { loge((NAME"no work root \n")); return STATUS_ACCESS_DENIED; }

	//
	// check parameter
	//
	PMsg_File file = buffer;
	if (file->path == NULL){ loge((NAME"invalid file path \n")); return STATUS_INVALID_PARAMETER; }

	//
	// check file path
	//
	if (!wcsstr(file->path, gWorkRoot.Buffer)){ loge((NAME"file isnot in work root \n")); return STATUS_INVALID_PARAMETER; }

	//
	// open file
	//
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, file->path);
	OBJECT_ATTRIBUTES oa = null;
	InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	PFILE_OBJECT obj = NULL;
	HANDLE hand = NULL;
	IO_STATUS_BLOCK iostatus;
	status = FltCreateFileEx(gFilter, gInstance, &hand, &obj, FILE_GENERIC_READ, &oa, &iostatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0, 0);
	if (!NT_SUCCESS(status)){ loge((NAME"open file failed. %x.%wZ", status, &name)); return status; }

	//
	// get permission
	//
	PPermission pm = NULL;
	status = getPermission(gInstance, obj, &pm);
	if (!NT_SUCCESS(status)){ loge((NAME"getPermission failed. %x.%wZ", status, &name)); FltClose(hand); return status; }

	//
	// compare permission
	//
	if (!pmIsUser(pm, &gUser)){ loge((NAME"only file Owner can set permission. %wZ", &name)); freePermission(pm); FltClose(hand); return status; }

	//
	// copy data
	//
	pm->code = file->pmCode;
	memcpy_s(&pm->user, sizeof(User), &file->user, sizeof(User));

	//
	// set permission
	//
	setPermission(gInstance, obj, pm);
	//
	// clear
	//
	freePermission(pm);
	FltClose(hand);
	return status;
}

NTSTATUS volume_query(void* buffer, unsigned long size, unsigned long *retlen){
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;

	//
	// check size
	//
	ULONG needSize = 25 * (sizeof(WCHAR) + sizeof(WCHAR)); // letter + type
	if (!buffer || size < needSize){
		*retlen = needSize;
		return STATUS_BUFFER_TOO_SMALL;
	}

	int i = 0;
	memset(buffer, 0, needSize);
	PWCHAR letters = buffer;
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (list->letter){
			letters[i++] = list->letter;
			letters[i++] = list->state;
		}
	}
	*retlen = (i / 2) * (sizeof(WCHAR) + sizeof(WCHAR));

	return status;
}

NTSTATUS workroot_get(void* buffer, unsigned long size, unsigned long *retlen){
	if (gWorkRoot.Length == 0) return STATUS_NOT_SUPPORTED;

	if (!buffer || size < gWorkRoot.Length + sizeof(WCHAR)){ *retlen = gWorkRoot.Length + sizeof(WCHAR); return STATUS_BUFFER_TOO_SMALL; }

	PWCHAR ptr = buffer;
	// letter
	ptr[0] = gWorkRootLetter;
	// guid
	memcpy_s(ptr + 1, size, gWorkRoot.Buffer, gWorkRoot.Length);
	return STATUS_SUCCESS;
}

NTSTATUS workroot_set(void* buffer, unsigned long size, unsigned long *retlen){

	if (!buffer || size < sizeof(WCHAR)){ *retlen = sizeof(WCHAR); return STATUS_INVALID_BUFFER_SIZE; }
	WCHAR letter = *((PWCHAR)buffer);
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;
	PVolumeList volume = NULL;
	//
	// found the volume
	//
	WCHAR tofind = letter > 0 ? letter : gWorkRootLetter;
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (!vl_ishasUser(list) &&
			list->letter == tofind){
			volume = list;
			break;
		}
	}
	if (!volume){ return STATUS_NOT_SUPPORTED; }

	//
	// save config
	//
	NTSTATUS status = STATUS_SUCCESS;
	if (letter > 0)
		status = IUtil->setConfig(gRegistry, L"WorkRoot", volume->GUID.Buffer, volume->GUID.Length, REG_SZ);
	else
		status = IUtil->setConfig(gRegistry, L"WorkRoot", &letter, sizeof(letter), REG_SZ);
	if (!NT_SUCCESS(status)){ loge((NAME"set config (%ws.%x) failed", L"WorkRoot", status)); return status; }

	//
	// set value
	//
	if (letter > 0){
		RtlCopyUnicodeString(&gWorkRoot, &list->GUID);
		gWorkRootLetter = letter;
		vl_setWorkRoot(volume);
		gInstance = volume->instance;
	}
	else{
		gWorkRoot.Length = 0;
		gWorkRootLetter = letter;
		vl_setNormal(volume);
		gInstance = NULL;
	}
	loge((NAME"set work root to: %wc", letter));
	return STATUS_SUCCESS;
}

//
// query driver state
//
NTSTATUS msg_query(void* buffer, unsigned long size, unsigned long *retlen){

	NTSTATUS status = STATUS_SUCCESS;
	if (!buffer || size < sizeof(MsgCode)){ *retlen = sizeof(MsgCode); return STATUS_BUFFER_TOO_SMALL; }

	*((MsgCode*)buffer) = currentMsg;	// send msg to application
	currentMsg = MsgCode_Null;			// clear msg
	return status;
}

// NTSTATUS workroot_get(void* buffer, unsigned long size, unsigned long *retlen){
// 
// 	NTSTATUS status = STATUS_SUCCESS;
// 	PLIST_ENTRY head = &gVolumeList;
// 	PVolumeList list = NULL;
// 
// }

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
};
