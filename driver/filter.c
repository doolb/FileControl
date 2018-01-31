#include "minidriver.h"

UNICODE_STRING gWorkRoot;	// the root path for dirver work
UNICODE_STRING gKeyRoot;		// the root path for key file
WCHAR _gWorkRootBuffer[] = L"\\??\\Volume{e6819827-fd02-11e7-8763-000c29dc5a92}\\"; // test root path

NTSTATUS oninit(){
	NTSTATUS status = STATUS_SUCCESS;

	// setup root path
	RtlInitUnicodeString(&gWorkRoot, _gWorkRootBuffer);


	return status;
}

void onexit(){

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