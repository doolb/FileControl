#include "minidriver.h"
#include "permission.h"
#include "util.h"
#include "filter.h"

UNICODE_STRING gWorkRoot;		// the root path for dirver work
UNICODE_STRING gKeyRoot;		// the root path for key file
static WCHAR gKeyRoot_Buffer[256];
WCHAR gWorkRootLetter;			// the letter of work root

LIST_ENTRY gVolumeList;

extern NPAGED_LOOKASIDE_LIST gPmLookasideList;

extern PFLT_FILTER gFilter;
extern PFLT_PORT gClient;

extern User gUser;

PFLT_INSTANCE gInstance;
HANDLE gRegistry;

//
// current driver state
//
MsgCode currentMsg = MsgCode_Null;

NTSTATUS oninit(PUNICODE_STRING _regPath){
	NTSTATUS status = STATUS_SUCCESS;

	OBJECT_ATTRIBUTES oa;
	ULONG retlen = 0;

	try{

		// init lookaside list for permission data
		ExInitializeNPagedLookasideList(&gPmLookasideList, NULL, NULL, 0, PM_SIZE, PM_TAG, 0);

		// open registry
		InitializeObjectAttributes(&oa, _regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&gRegistry, KEY_READ, &oa);
		if (!NT_SUCCESS(status)){ loge((NAME"open registry failed. %x:%wZ \n", status, _regPath)); leave; }


		//
		// work root path
		//
		retlen = 0; // we dont know the size
		status = IUtil->getConfig(gRegistry, L"WorkRoot", &gWorkRoot.Buffer, &retlen);
		if (!NT_SUCCESS(status)){ loge((NAME"get config: WorkRoot failed. %x \n", status)); leave; }
		if (retlen > 0){
			gWorkRoot.Length = (USHORT)retlen;
			gWorkRoot.MaximumLength = sizeof(gKeyRoot_Buffer);
			log((NAME"work root dir: %wZ \n", &gWorkRoot));
		}

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
	}

	return status;
}

void onexit(){

	if (gRegistry) ZwClose(gRegistry); gRegistry = NULL;

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

	//
	// save volume letter
	//
	if (ctx->Name.Length == 2 * sizeof(WCHAR))
		list->letter = ctx->Name.Buffer[0];
	else
		list->letter = 0;

	if (ctx->prop->DeviceCharacteristics & FILE_REMOVABLE_MEDIA) {
		//
		// can load user key 
		//
		NTSTATUS status = IUserKey->read(list->instance, &list->GUID, &list->key);
		if (NT_SUCCESS(status)){
			logw((NAME"find a user"));
			vl_sethasUser(list);
		}
		else
		vl_setRemove(list);
	}
	else{
		//
		// is work root
		//
		if (gWorkRoot.Length > 0 && wcsstr(gWorkRoot.Buffer, list->GUID.Buffer))		{
			gWorkRootLetter = list->letter;	// save the letter
			vl_setWorkRoot(list);
			gInstance = instance;			// save the instance
		}
		else
			vl_setNormal(list);
	}

	return (PLIST_ENTRY)list;
}

NTSTATUS onstart(PVolumeContext ctx, PFLT_INSTANCE instance){
	NTSTATUS status = STATUS_SUCCESS;

	if (ctx->Name.Buffer && ctx->Name.Buffer[0] == L'C'){
		logw((NAME"we skip the System volume."));
		return status = STATUS_FLT_DO_NOT_ATTACH;
	}

	//
	// add this volume to list
	//
	PLIST_ENTRY list = createVolumeList(ctx, instance);
	if (!list){ loge((NAME"createVolumeList failed. %wZ", &ctx->GUID)); return STATUS_INSUFFICIENT_RESOURCES; }
	InsertHeadList(&gVolumeList, list);
	//sendMsg(MsgCode_Volume_Query); // notify application volume changed
	return status;
}

void onstop(PVolumeContext ctx){
	// no guid, skip it
	if (ctx->GUID.Length == 0) return;

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
			// sendMsg(MsgCode_Volume_Query); // notify application volume changed
			break;
		}
	}
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

	currentMsg = code;
	
	return status;
}