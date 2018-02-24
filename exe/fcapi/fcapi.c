#define FC_IMPLEMENT
#include "fcapi.h"
#pragma comment(lib,"fltlib.lib")

static HANDLE gPort = INVALID_HANDLE_VALUE;
static HANDLE gDaemonPort = INVALID_HANDLE_VALUE;
static bool gIsDaemon;

void CheckError(HRESULT ret){
	WCHAR buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ret, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL);
	MessageBox(0, buf, 0, 0);
}

void fc_init(bool isdaemon){
	gIsDaemon = isdaemon;
}

bool fc_open(){
	HRESULT ret = 0;
	//
	// open communication port
	//
	PHANDLE hand = gIsDaemon ? &gDaemonPort : &gPort;
	if (*hand == INVALID_HANDLE_VALUE){
		ret = FilterConnectCommunicationPort(gIsDaemon ? FC_DAEMON_PORT_NAME : FC_PORT_NAME, 0, NULL, 0, NULL, hand);
		if (ret){
			CheckError(ret);
			return false;
		}
	}

	return true;
}

void fc_close(){

	PHANDLE hand = gIsDaemon ? &gDaemonPort : &gPort;

	if (*hand != INVALID_HANDLE_VALUE)
		CloseHandle(*hand);
	*hand = INVALID_HANDLE_VALUE;
}

HRESULT fc_send(PMsg msg){

	PHANDLE hand = gIsDaemon ? &gDaemonPort : &gPort;

	DWORD ret = 0;
	HRESULT rst = S_OK;
	Msg out = { 0 };
	rst = FilterSendMessage(*hand, msg, sizeof(Msg), &out, sizeof(Msg), &ret);
	if (rst){
		CheckError(rst);
	}

	memcpy_s(msg, sizeof(Msg), &out, sizeof(Msg));
	return rst;
}
HRESULT fc_listen(PMsgCode code){
	if (gDaemonPort == INVALID_HANDLE_VALUE) return E_INVALID_PROTOCOL_FORMAT;

	HRESULT rst = S_OK;
	MsgData m = { 0 };
	rst = FilterGetMessage(gDaemonPort, (PFILTER_MESSAGE_HEADER)&m, sizeof(MsgData), NULL);
	if (rst){
		CheckError(rst);
	}
	*code = m.code;
	return rst;
}

FC_API struct _IFc IFc[1] = {
	fc_init,

	fc_open,
	fc_close,

	fc_send,
	fc_listen
};