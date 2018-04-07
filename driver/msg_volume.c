#include <fltKernel.h>
#include "msg.h"
#include "filter.h"

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
	*retlen = gWorkRoot.Length + sizeof(WCHAR);
	return STATUS_SUCCESS;
}

NTSTATUS workroot_set(void* buffer, unsigned long size, unsigned long *retlen){
	//
	// check admin 
	//
	if (!FlagOn(gAdmin.state, Admin_Login)){ return STATUS_ACCESS_DENIED; }

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
		gWorkRootLetter = 0;
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
