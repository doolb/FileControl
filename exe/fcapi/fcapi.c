#define FC_IMPLEMENT
#include "fcapi.h"
#pragma comment(lib,"fltlib.lib")

#include <stdlib.h>
#include <assert.h>

static HANDLE gPort = INVALID_HANDLE_VALUE;

void CheckError(HRESULT ret){
	WCHAR buf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ret, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), NULL);
	MessageBox(0, buf, 0, 0);
}

bool fc_open(bool isdaemon){
	HRESULT ret = 0;
	//
	// open communication port
	//
	if (gPort == INVALID_HANDLE_VALUE){
		ret = FilterConnectCommunicationPort(isdaemon ? FC_DAEMON_PORT_NAME : FC_PORT_NAME, 0, NULL, 0, NULL, &gPort);
		if (ret){
			CheckError(ret);
			return false;
		}
	}
	else
		return false;

	return true;
}

void fc_close(){
	if (gPort != INVALID_HANDLE_VALUE)	CloseHandle(gPort);
	gPort = INVALID_HANDLE_VALUE;
}

HRESULT fc_send(MsgCode msg, PVOID buffer, ULONG size, PULONG retlen){

	if (gPort == INVALID_HANDLE_VALUE) return E_INVALID_PROTOCOL_FORMAT;
	if (!retlen) return E_INVALID_PROTOCOL_FORMAT;

	HRESULT		rst = S_OK;
	rst = FilterSendMessage(gPort, &msg, sizeof(MsgCode), buffer, size, retlen);
	if (rst)		CheckError(rst);
	return rst;
}
HRESULT fc_listen(PMsgCode code){
	if (gPort == INVALID_HANDLE_VALUE) return E_INVALID_PROTOCOL_FORMAT;

	HRESULT rst = S_OK;
	MsgData m = { 0 };

	rst = FilterGetMessage(gPort, (PFILTER_MESSAGE_HEADER)&m, sizeof(MsgData), NULL);
	if (rst)CheckError(rst);

	*code = m.code;
	return rst;
}

/************************************************************************/
/* extern function
/************************************************************************/
int fc_query_user(PUser *users){
	if (gPort == INVALID_HANDLE_VALUE) return E_INVALID_PROTOCOL_FORMAT;

	HRESULT rst = S_OK;
	ULONG retlen = 0;

	MsgCode msg = MsgCode_User_Query;
	//
	// get the buffer size first
	//
	rst = FilterSendMessage(gPort, &msg, sizeof(MsgCode), NULL, 0, &retlen);

	// is no user in system
	if (!retlen){
		return S_FALSE;
	}

	//
	// now we need allocate memory for save data
	//
	PUser buff = malloc(retlen);
	if (!buff){
		return E_OUTOFMEMORY;
	}

	//
	// now we can query the user data
	//
	rst = FilterSendMessage(gPort, &msg, sizeof(MsgCode), buff, retlen, &retlen);
	if (rst){ CheckError(rst); }

	*users = buff;
	return retlen / sizeof(User);
}
FC_API struct _IFc IFc[1] = {
	fc_open,
	fc_close,

	fc_send,
	fc_listen,

	fc_query_user
};