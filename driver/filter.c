#include "minidriver.h"
#include "permission.h"

UNICODE_STRING gWorkRoot;	// the root path for dirver work
UNICODE_STRING gKeyRoot;		// the root path for key file

NTSTATUS oninit(PUNICODE_STRING _regPath){
	NTSTATUS status = STATUS_SUCCESS;

	HANDLE hand = NULL;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING name;
	PUCHAR buff = NULL;
	ULONG retlen = 0;

	try{
		//
		// work root path
		//
		// open registry
		InitializeObjectAttributes(&oa, _regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&hand, KEY_READ, &oa);
		if (!NT_SUCCESS(status)){ loge((NAME"open registry failed. %x:%wZ", status, _regPath)); leave; }

		// value name
		RtlInitUnicodeString(&name, L"WorkRoot");	

		// get the value date len
		status = ZwQueryValueKey(hand, &name, KeyValuePartialInformation, NULL, 0, &retlen);
		if (retlen == 0){ loge((NAME"value length invalid. %x:%wZ", status, &name)); leave; }

		// allocate memory
		buff = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + retlen, NAME_TAG);
		if (!buff){ loge((NAME"ExAllocatePoolWithTag failed.")); leave; }
		gWorkRoot.Buffer = ExAllocatePoolWithTag(NonPagedPool, retlen, NAME_TAG);
		if (!gWorkRoot.Buffer){ loge((NAME"ExAllocatePoolWithTag failed.")); leave; }

		// get the value date
		status = ZwQueryValueKey(hand, &name, KeyValuePartialInformation, buff, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + retlen, &retlen);
		if (!NT_SUCCESS(status)){ loge((NAME"open registry failed. %x:%wZ", status, _regPath)); leave; }

		// save value
		memcpy_s(gWorkRoot.Buffer, retlen, ((PKEY_VALUE_PARTIAL_INFORMATION)buff)->Data, retlen);
		gWorkRoot.Length = gWorkRoot.MaximumLength = (USHORT)retlen;
		log((NAME"work root dir: %wZ", &gWorkRoot));
	}
	finally{
		if (hand) ZwClose(hand); hand = NULL; 
		if (buff) ExFreePoolWithTag(buff, NAME_TAG); buff = NULL;
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

void onstop(){

}
NTSTATUS onfilter(){
	NTSTATUS status = STATUS_SUCCESS;


	return status;
}
NTSTATUS onmsg(){
	NTSTATUS status = STATUS_SUCCESS;


	return status;
}