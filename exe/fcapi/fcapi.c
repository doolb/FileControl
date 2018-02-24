#define FC_IMPLEMENT
#include "fcapi.h"
#pragma comment(lib,"fltlib.lib")

static HANDLE gPort = INVALID_HANDLE_VALUE;
static HANDLE gDaemonPort = INVALID_HANDLE_VALUE;

void CheckError(HRESULT ret){
	WCHAR buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ret, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL);
	MessageBox(0, buf, 0, 0);
}

FC_API bool fc_open(bool isdaemon){
	HRESULT ret = 0;
	//
	// open communication port
	//
	PHANDLE hand = isdaemon ? &gDaemonPort : &gPort;
	if (*hand == INVALID_HANDLE_VALUE){
		ret = FilterConnectCommunicationPort(isdaemon ? FC_DAEMON_PORT_NAME : FC_PORT_NAME, 0, NULL, 0, NULL, hand);
		if (ret){
			CheckError(ret);
			return false;
		}
	}

	return true;
}

FC_API void fc_close(bool isdaemon){

	PHANDLE hand = isdaemon ? &gDaemonPort : &gPort;

	if (*hand != INVALID_HANDLE_VALUE)
		CloseHandle(*hand);
	*hand = INVALID_HANDLE_VALUE;
}

FC_API int fc_send(bool isdaemon, PWCH msg, int len, PWCH out, int outLen){

	PHANDLE hand = isdaemon ? &gDaemonPort : &gPort;

	DWORD ret = 0;
	HRESULT rst = S_OK;
	rst = FilterSendMessage(*hand, msg, len*sizeof(WCHAR), out, outLen*sizeof(WCHAR), &ret);
	if (rst){
		CheckError(rst);
	}
	return ret;
}