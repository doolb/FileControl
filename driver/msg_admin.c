#include <fltKernel.h>
#include "msg.h"
#include "filter.h"

int isadmin(PGUID gid){
	return gid->Data1 == 0 && gid->Data2 == 0 && gid->Data3 == 0 &&
		gid->Data4[0] == 0 && gid->Data4[1] == 0 && gid->Data4[2] == 0 && gid->Data4[3] == 0 &&
		gid->Data4[4] == 0 && gid->Data4[5] == 0 && gid->Data4[6] == 0 && gid->Data4[7] == 1;
}


NTSTATUS admin_new(void* buffer, unsigned long size, unsigned long *retlen){

	// is has admin
	//if (FlagOn(gAdmin.state, Admin_Valid)){ return STATUS_ACCESS_DENIED; }

	// check size
	if (!buffer || size < sizeof(Msg_Admin_Registry)){
		*retlen = sizeof(Msg_Admin_Registry);
		return STATUS_BUFFER_TOO_SMALL;
	}

	NTSTATUS status = STATUS_SUCCESS;
	PMsg_Admin_Registry reg = buffer;

	//
	// found volume for registry
	//
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList volume = NULL, list = NULL;
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (list->letter == reg->letter){
			volume = list;
			break;
		}
	}
	if (volume == NULL){
		return STATUS_NOT_SUPPORTED;
	}

	//
	// registry key
	//
	status = IAdminKey->registry(&volume->GUID, reg, &volume->adminKey);
	if (!NT_SUCCESS(status)){ loge((NAME"registry admin key failed. %wc", reg->letter)); return status; }

	//
	// write key
	//
	status = IAdminKey->write(volume->instance, &volume->GUID, &volume->adminKey);
	if (!NT_SUCCESS(status)){ loge((NAME"write admin key failed. %wc", reg->letter)); return status; }

	return status;
}

NTSTATUS admin_query(void* buffer, unsigned long size, unsigned long *retlen){
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(retlen);

	*retlen = sizeof(ULONG);
	if (!buffer || size < sizeof(ULONG)){ return STATUS_BUFFER_TOO_SMALL; }

	PULONG ret = buffer;
	*ret = FlagOn(gAdmin.state, Admin_Valid);

	return STATUS_SUCCESS;
}

NTSTATUS admin_set(void* buffer, unsigned long size, unsigned long *retlen){

	NTSTATUS status = STATUS_SUCCESS;

	// check size
	if (!buffer || size < sizeof(User)){ *retlen = sizeof(User); return STATUS_BUFFER_TOO_SMALL; }

	PUser user = buffer;

	// is user is admin
	if (!isadmin(&user->gid)){ return STATUS_ACCESS_DENIED; }

	//
	// found volume for registry
	//
	PLIST_ENTRY head = &gVolumeList;
	PVolumeList volume = NULL, list = NULL;
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		list = CONTAINING_RECORD(e, VolumeList, list);
		if (list->adminKey.valid &&
			memcmp(&list->adminKey.user.uid, &user->uid, sizeof(GUID)) == 0){
			volume = list;
			break;
		}
	}
	if (volume == NULL){
		return STATUS_INVALID_PARAMETER;
	}

	// is a admin user on this volume
	if (!volume->adminKey.valid){
		return STATUS_NOT_SUPPORTED;
	}

	//
	// setup admin
	//
	memcpy_s(&gAdmin.admin, sizeof(User), &volume->adminKey.user, sizeof(User));
	gAdmin.key.modulus = volume->adminKey.pub.modulus;
	gAdmin.key.exponent = volume->adminKey.pub.exponent;
	SetFlag(gAdmin.state, Admin_Valid);

	//
	// save data
	//
	IUtil->setConfig(gRegistry, L"Module", &gAdmin.key.modulus, sizeof(long long), REG_QWORD);
	IUtil->setConfig(gRegistry, L"Exponent", &gAdmin.key.exponent, sizeof(long long), REG_QWORD);

	return status;
}