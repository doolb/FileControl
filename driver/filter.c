#include "minidriver.h"
#include "permission.h"
#include "util.h"
#include "filter.h"

UNICODE_STRING gWorkRoot;		// the root path for dirver work
UNICODE_STRING gKeyRoot;		// the root path for key file
static WCHAR gKeyRoot_Buffer[256];
WCHAR gWorkRootLetter;			// the letter of work root

LIST_ENTRY gVolumeList;

KSPIN_LOCK gFilterLock;

extern NPAGED_LOOKASIDE_LIST gPmLookasideList;

extern PFLT_FILTER gFilter;
extern PFLT_PORT gDaemonClient;

extern User gUser;

PFLT_INSTANCE gInstance;

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
	if (wcsstr(gWorkRoot.Buffer, list->GUID.Buffer))		{
		gWorkRootLetter = list->letter;	// save the letter
		list->isWorkRoot = TRUE;		// set flag
		gInstance = instance;			// save the instance
	}

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

			//
			// is work root
			//
			if (list->instance == gInstance) {
				gWorkRootLetter = 0;
				gInstance = NULL;
			}

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

extern NTSTATUS(*MsgHandle[MsgCode_Max + 1])(void* buffer, unsigned long size, unsigned long *retlen);
NTSTATUS onmsg(MsgCode msg, PVOID buffer, ULONG size, PULONG retlen){
	ASSERT(retlen);

	NTSTATUS status = STATUS_SUCCESS;
	*retlen = 0;

	if (MsgHandle[msg] != NULL) status = MsgHandle[msg](buffer, size, retlen);
	else status = STATUS_NOT_SUPPORTED;

	return status;
}

NTSTATUS sendMsg(MsgCode code){
	NTSTATUS status = STATUS_SUCCESS;

	//
	// precheck
	//
	if (gKeyRoot.Length != 0 && code == MsgCode_User_Login) return status;

	if (gDaemonClient){

		LARGE_INTEGER timeout;
		timeout.QuadPart = 1000;

		log((NAME"send message to application. %x", code));
		status = FltSendMessage(gFilter, &gDaemonClient, &code, sizeof(MsgCode), NULL, NULL, &timeout);
		log((NAME"send message to application. %x.%x", code, status));
	}
	return status;
}