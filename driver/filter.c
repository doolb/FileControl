#include "minidriver.h"
#include "permission.h"
#include "util.h"

static BOOL gPause = FALSE;
static UNICODE_STRING gWorkRoot;	// the root path for dirver work
static UNICODE_STRING gKeyRoot;		// the root path for key file

NTSTATUS oninit(PUNICODE_STRING _regPath){
	NTSTATUS status = STATUS_SUCCESS;

	HANDLE hand = NULL;
	OBJECT_ATTRIBUTES oa;
	ULONG retlen = 0;

	try{
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

		gPause = FALSE;
		IUtil->setConfig(hand, L"Pause", &gPause, sizeof(BOOL), REG_DWORD);
	}
	finally{
		if (hand) ZwClose(hand); hand = NULL;
	}

	return status;
}

void onexit(){
	if (gWorkRoot.Buffer){ ExFreePoolWithTag(gWorkRoot.Buffer, NAME_TAG); gWorkRoot.Buffer = NULL; }

}

NTSTATUS onstart(PVolumeContext ctx){
	NTSTATUS status = STATUS_SUCCESS;

	if (wcsstr(ctx->prop->RealDeviceName.Buffer, L"HarddiskVolume1")){
		return status = STATUS_FLT_DO_NOT_ATTACH;
	}

	return status;
}

void onstop(PVolumeContext ctx){
	UNREFERENCED_PARAMETER(ctx);
}
NTSTATUS onfilter(PFLT_FILE_NAME_INFORMATION info, PUNICODE_STRING guid){

	ASSERT(info);
	ASSERT(guid);

	// is pause
	if (gPause)
		return FLT_NO_NEED;

	// is the volume for work
	if (!wcsstr(gWorkRoot.Buffer, guid->Buffer))
		return FLT_NO_NEED;

	// is user login
	if (gKeyRoot.Length == 0)
		return STATUS_ACCESS_DENIED;

	return STATUS_SUCCESS;
}
NTSTATUS onmsg(){
	NTSTATUS status = STATUS_SUCCESS;


	return status;
}