#include "minidriver.h"
#include "permission.h"
#include "util.h"
#include "filter.h"

static BOOL gPause = FALSE;
static UNICODE_STRING gWorkRoot;		// the root path for dirver work
static UNICODE_STRING gKeyRoot;		// the root path for key file
static WCHAR gKeyRoot_Buffer[256];

static LIST_ENTRY gVolumeList;

KSPIN_LOCK gFilterLock;

extern NPAGED_LOOKASIDE_LIST gPmLookasideList;

extern PFLT_FILTER gFilter;
extern PFLT_PORT gDaemonClient;

extern User gUser;

NTSTATUS oninit(PUNICODE_STRING _regPath){
	NTSTATUS status = STATUS_SUCCESS;

	HANDLE hand = NULL;
	OBJECT_ATTRIBUTES oa;
	ULONG retlen = 0;

	try{
		// init lock
		KeInitializeSpinLock(&gFilterLock);

		// init lookaside list for permission data
		ExInitializeNPagedLookasideList(&gPmLookasideList, NULL, NULL, 0, PM_SIZE, PM_TAG, 0);

		// open registry
		InitializeObjectAttributes(&oa, _regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&hand, KEY_READ, &oa);
		if (!NT_SUCCESS(status)){ loge((NAME"open registry failed. %x:%wZ \n", status, _regPath)); leave; }


		//
		// work root path
		//
		retlen = 0; // we dont know the size
		status = IUtil->getConfig(hand, L"WorkRoot", &gWorkRoot.Buffer, &retlen);
		if (!NT_SUCCESS(status)){ loge((NAME"get config: WorkRoot failed. %x \n", status)); leave; }
		gWorkRoot.Length = gWorkRoot.MaximumLength = (USHORT)retlen;
		log((NAME"work root dir: %wZ \n", &gWorkRoot));

		//
		// driver status
		//
		retlen = sizeof(BOOL); // dont need allocate buffer
		status = IUtil->getConfig(hand, L"Pause", (PVOID)&gPause, &retlen);
		log((NAME"driver pause : %x \n", gPause));

		//
		// setup key root
		//
		RtlInitEmptyUnicodeString(&gKeyRoot, gKeyRoot_Buffer, sizeof(gKeyRoot_Buffer));

		//
		// init volume list head
		//
		InitializeListHead(&gVolumeList);

		// test write config
		//gPause = FALSE;
		//IUtil->setConfig(hand, L"Pause", &gPause, sizeof(BOOL), REG_DWORD);


	}
	finally{
		if (hand) ZwClose(hand); hand = NULL;
	}

	return status;
}

void onexit(){
	if (gWorkRoot.Buffer){ ExFreePoolWithTag(gWorkRoot.Buffer, NAME_TAG); gWorkRoot.Buffer = NULL; }

	//
	// delete lookaside list
	//
	ExDeleteNPagedLookasideList(&gPmLookasideList);

	//
	// clear volume list
	//
	PLIST_ENTRY head = &gVolumeList;
	for (PLIST_ENTRY e = RemoveHeadList(head); e != head; e = RemoveHeadList(head))
		ExFreePoolWithTag(e, FLT_TAG);
}


static PLIST_ENTRY createVolumeList(PVolumeContext ctx, PFLT_INSTANCE instance){
	ASSERT(ctx);

	NTSTATUS status = STATUS_SUCCESS;

	//
	// allocate memory
	//
	PVolumeList list = ExAllocatePoolWithTag(NonPagedPool, sizeof(VolumeList), FLT_TAG);
	if (!list){ loge((NAME"allocate memory failed.")); return NULL; }
	memset(list, 0, sizeof(VolumeList));

	//
	// save instance
	//
	list->instance = instance;

	//
	// copy data
	//
	RtlInitEmptyUnicodeString(&list->GUID, list->_GUID_Buffer, sizeof(WCHAR) * GUID_SIZE);
	RtlCopyUnicodeString(&list->GUID, &ctx->GUID);
	list->type = ctx->prop->DeviceCharacteristics;

	//
	// save volume letter
	//
	if (ctx->Name.Length == 2 * sizeof(WCHAR))
		list->letter = ctx->Name.Buffer[0];
	else
		list->letter = 0;

	//
	// load user key 
	//
	status = IUserKey->read(list->instance, &list->GUID, &list->key);
	if (NT_SUCCESS(status)){
		logw((NAME"find a user"));
		list->isHasUser = TRUE;
	}

	//
	// is work root
	//
	if (wcsstr(gWorkRoot.Buffer, list->GUID.Buffer))		list->isWorkRoot = TRUE;

	return (PLIST_ENTRY)list;
}

NTSTATUS onstart(PVolumeContext ctx, PFLT_INSTANCE instance){
	NTSTATUS status = STATUS_SUCCESS;

	if (wcsstr(ctx->prop->RealDeviceName.Buffer, L"HarddiskVolume1")){
		return status = STATUS_FLT_DO_NOT_ATTACH;
	}

	//
	// lock operation
	//
	KLOCK_QUEUE_HANDLE hand;
	KeAcquireInStackQueuedSpinLock(&gFilterLock, &hand);

	//
	// add this volume to list
	//
	PLIST_ENTRY list = createVolumeList(ctx, instance);
	if (!list){ loge((NAME"createVolumeList failed. %wZ", &ctx->GUID)); return STATUS_INSUFFICIENT_RESOURCES; }
	InsertHeadList(&gVolumeList, list);


	//
	// unlock 
	//
	KeReleaseInStackQueuedSpinLock(&hand);

	return status;
}

void onstop(PVolumeContext ctx){
	// no guid, skip it
	if (ctx->GUID.Length == 0) return;

	//
	// lock operation
	//
	KLOCK_QUEUE_HANDLE hand;
	KeAcquireInStackQueuedSpinLock(&gFilterLock, &hand);

	PLIST_ENTRY head = &gVolumeList;
	for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
		//
		// remove the same context from list
		//
		PVolumeList list = CONTAINING_RECORD(e, VolumeList, list);
		if (RtlCompareUnicodeString(&ctx->GUID, &list->GUID, FALSE) == 0){

			logw((NAME"remove volume : %wZ", &ctx->GUID));

			RemoveEntryList(e);
			ExFreePoolWithTag(e, FLT_TAG);
			break;
		}
	}


	//
	// unlock 
	//
	KeReleaseInStackQueuedSpinLock(&hand);
}
NTSTATUS onfilter(PFLT_FILE_NAME_INFORMATION info, PUNICODE_STRING guid){

	ASSERT(info);
	ASSERT(guid);

	// is pause
	if (gPause){ logw((NAME"driver is paused.")); return FLT_NO_NEED; }

	// is the volume for work
	if (!wcsstr(gWorkRoot.Buffer, guid->Buffer)) { return FLT_NO_NEED; }


	// is user login
	if (gKeyRoot.Length == 0){
		loge((NAME"user must be login."));

		// send message to application
		sendMsg(MsgCode_User_Login);

		return STATUS_ACCESS_DENIED;
	}

	return STATUS_SUCCESS;
}
NTSTATUS onmsg(MsgCode msg, PVOID buffer, ULONG size, PULONG retlen){
	ASSERT(retlen);

	NTSTATUS status = STATUS_SUCCESS;
	*retlen = 0;

	PLIST_ENTRY head = &gVolumeList;
	PVolumeList list = NULL;

	switch (msg)
	{
	case MsgCode_User_Query:{
#pragma region MsgCode_User_Query
		int count = 0;
		//
		// get the all user count
		//	
		for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
			list = CONTAINING_RECORD(e, VolumeList, list);
			if (list->isHasUser)
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
				if (list->isHasUser){
					memcpy_s((PVOID)buff, sizeof(User), &list->key.user, sizeof(User));

					buff += sizeof(User);
				}
			}
		}
		else{
			status = STATUS_BUFFER_TOO_SMALL;
		}
#pragma endregion
		break;
	}

	case MsgCode_Volume_Query:{
#pragma region MsgCode_Volume_Query
		//
		// check size
		//
		ULONG needSize = 26 * sizeof(WCHAR);
		if (!buffer || size < needSize){
			status = STATUS_BUFFER_TOO_SMALL;
			*retlen = needSize;
			break;
		}

		int i = 0;
		memset(buffer, 0, needSize);
		PWCHAR letters = buffer;
		for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
			list = CONTAINING_RECORD(e, VolumeList, list);
			if (list->letter &&
				!list->isWorkRoot)
				letters[i++] = list->letter;
		}
		*retlen = i * sizeof(WCHAR);
#pragma endregion
		break;
	}
	case MsgCode_User_Registry:{
#pragma region MsgCode_User_Registry
		if (!buffer || size < sizeof(Msg_User_Registry)){ *retlen = sizeof(Msg_User_Registry); status = STATUS_BUFFER_TOO_SMALL; break; }

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
		if (!volume){ status = FLT_NO_USER; break; }

		//
		// registry user
		//
		PUserKey key = IUserKey->registry(&volume->GUID, reg->name, reg->group, reg->password);
		if (!key){ loge((NAME"registry user failed. %ws", reg->letter)); break; }

		//
		// write user key
		//
		status = IUserKey->write(list->instance, &volume->GUID, key);
		if (!NT_SUCCESS(status)){ loge((NAME"write user key failed. %ws", reg->letter)); break; }

		//
		// save key to volume lsit
		//
		memcpy_s(&volume->key, sizeof(UserKey), key, sizeof(UserKey));
		volume->isHasUser = TRUE;

		//
		// login user
		//
		RtlCopyUnicodeString(&gKeyRoot, &volume->GUID);
		memcpy_s(&gUser, sizeof(User), &key->user, sizeof(User));

		logw((NAME"registry user success, login in. %ws", reg->letter));
		status = STATUS_SUCCESS;
#pragma endregion
		break;
	}
	case MsgCode_User_Login:{
#pragma region MsgCode_User_Login
		if (!buffer || size < sizeof(Msg_User_Login)){ *retlen = sizeof(Msg_User_Login); status = STATUS_BUFFER_TOO_SMALL; break; }

		PMsg_User_Login login = buffer;
		PVolumeList volume = NULL;
		//
		// found the volume
		//
		for (PLIST_ENTRY e = head->Blink; e != head; e = e->Blink){
			list = CONTAINING_RECORD(e, VolumeList, list);
			if (list->isHasUser &&
				memcmp(&list->key.user.uid, &login->user.uid, sizeof(GUID)) == 0){
				volume = list;
				break;
			}
		}
		if (!volume){ status = FLT_NO_USER; break; }

		//
		// compare password
		//
		UINT8 hash[HASH_SIZE];
		IUtil->hash((PUINT8)login->password, wcsnlen(login->password, PM_NAME_MAX) * sizeof(WCHAR), hash);
		if (memcmp(hash, volume->key.passwd, HASH_SIZE) != 0){ status = FLT_INVALID_PASSWORD; break; }

		//
		// login user
		//
		RtlCopyUnicodeString(&gKeyRoot, &volume->GUID);
		memcpy_s(&gUser, sizeof(User), &volume->key.user, sizeof(User));

		logw((NAME"user login in. %ws", volume->letter));
		status = STATUS_SUCCESS;
#pragma endregion
		break;
	}
	default:
		break;
	}

	return status;
}

NTSTATUS sendMsg(MsgCode code){
	NTSTATUS status = STATUS_SUCCESS;
	if (gDaemonClient){
		status = FltSendMessage(gFilter, &gDaemonClient, &code, sizeof(MsgCode), NULL, NULL, NULL);
		log((NAME"send message to application. %x", code));
	}
	return status;
}