#define FC_IMPLEMENT
#include "fcapi.h"
#pragma comment(lib,"fltlib.lib")

static HANDLE gPort = INVALID_HANDLE_VALUE;

void CheckError(HRESULT ret){
	WCHAR buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ret, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL);
	MessageBox(0, buf, 0, 0);
}

FC_API bool fc_open(){
	HRESULT ret = 0;
	//
	// open communication port
	//
	if (gPort == INVALID_HANDLE_VALUE){
		ret = FilterConnectCommunicationPort(FC_PORT_NAME, 0, NULL, 0, NULL, &gPort);
		if (ret){
			CheckError(ret);
			return false;
		}
	}
		
	return true;
}

FC_API void fc_close(){
	if (gPort != INVALID_HANDLE_VALUE)
		CloseHandle(gPort);
	gPort = INVALID_HANDLE_VALUE;
}

FC_API int fc_send(PWCH msg, int len, PWCH out, int outLen){
	DWORD ret = 0;
	HRESULT rst = S_OK;
	rst = FilterSendMessage(gPort, msg, len*sizeof(WCHAR), out, outLen*sizeof(WCHAR), &ret);
	if (rst){
		CheckError(rst);
	}
	return ret;
}