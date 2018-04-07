#include <fltKernel.h>
#include "msg.h"
#include "filter.h"

extern int isadmin(PGUID gid);

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
		if (vl_ishasUser(list)) // is has user
			count++;
		if (list->adminKey.valid) // is has admin
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

			// copy user data
			if (vl_ishasUser(list)){
				memcpy_s((PVOID)buff, sizeof(User), &list->key.user, sizeof(User));
				buff += sizeof(User);
			}

			// copy admin data
			if (list->adminKey.valid){
				memcpy_s((PVOID)buff, sizeof(User), &list->adminKey.user, sizeof(User));
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
	//
	// check admin 
	//
	if (!FlagOn(gAdmin.state, Admin_Login)){ return STATUS_ACCESS_DENIED; }

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
		if (list->adminKey.valid &&
			memcmp(&list->adminKey.user.uid, &login->user.uid, sizeof(GUID)) == 0){
			volume = list;
			break;
		}
	}
	if (!volume){ return FLT_NO_USER; }


	UINT8 hash[HASH_SIZE];
	IUtil->hash((PUINT8)login->password, wcsnlen(login->password, PM_NAME_MAX) * sizeof(WCHAR), hash);
	if (isadmin(&login->user.gid))
	{
		if (memcmp(hash, volume->adminKey.passwd, HASH_SIZE) != 0){ loge((NAME"uncorrect password.\n")); return FLT_INVALID_PASSWORD; }

		memcpy_s(&gAdmin.admin, sizeof(User), &login->user, sizeof(User));
		SetFlag(gAdmin.state, Admin_Login);
	}
	else
	{
		//
		// compare password
		//
		if (memcmp(hash, volume->key.passwd, HASH_SIZE) != 0){ loge((NAME"uncorrect password.\n")); return FLT_INVALID_PASSWORD; }

		//
		// login user
		//
		RtlCopyUnicodeString(&gKeyRoot, &volume->GUID);
		memcpy_s(&gUser, sizeof(User), &volume->key.user, sizeof(User));
		vl_setKeyRoot(volume);
	}


	loge((NAME"user login in. %ws", volume->letter));
	status = STATUS_SUCCESS;
	return status;
}
NTSTATUS user_delete(void* buffer, unsigned long size, unsigned long *retlen){

	//
	// check admin 
	//
	if (!FlagOn(gAdmin.state, Admin_Login)){ return STATUS_ACCESS_DENIED; }

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

	if (!buffer || size < sizeof(User)){ *retlen = sizeof(User); return STATUS_BUFFER_TOO_SMALL; }
	PUser user = buffer;

	PLIST_ENTRY head = &gVolumeList;
	PVolumeList volume = NULL, list = NULL;
	//
	// found the volume
	//
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (vl_isKeyRoot(list) && !isadmin(&user->gid) &&
			memcmp(&list->key.user.uid, &user->uid, sizeof(GUID)) == 0){
			volume = list;
			break;
		}
		if (list->adminKey.valid && isadmin(&user->gid) && FlagOn(gAdmin.state, Admin_Login) &&
			memcmp(&list->adminKey.user.uid, &user->uid, sizeof(GUID)) == 0){
			volume = list;
			break;
		}
	}
	if (!volume){ return FLT_NO_USER; }

	if (isadmin(&user->gid))
		ClearFlag(gAdmin.state, Admin_Login);
	else{
		vl_sethasUser(volume);
		gKeyRoot.Length = 0;
	}
	return STATUS_SUCCESS;
}
