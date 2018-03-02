#define FC_STATIC
#include "fcapi.h"
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <objbase.h>

ULONG retlen = 0;
HRESULT rst = S_OK;
PUser users = NULL;

MsgCode msg = MsgCode_Null;

void test_pause();
void test_listen();

void __cdecl main(){

	try{
		if (!IFc->open(true)) leave;

		//test_pause();
		test_listen();
	}
	finally{
		if (users) free(users);

		IFc->close();
	}


	getchar();
}


void test_pause(){
	BOOL pause = FALSE;

	rst = IFc->send(MsgCode_GetPause, &pause, sizeof(BOOL), &retlen);
	if (FAILED(rst)) return;

	printf("driver is %s.\n", pause ? "paused" : "runing");

	pause = !pause;
	rst = IFc->send(MsgCode_SetPause, &pause, sizeof(BOOL), &retlen);
	if (FAILED(rst)) return;

	printf("driver is %s.\n", pause ? "paused" : "runing");

	printf("test invalid parameter\n");
	rst = IFc->send(MsgCode_SetPause, NULL, sizeof(BOOL), &retlen);
	rst = IFc->send(MsgCode_GetPause, &pause, 0, &retlen);
	rst = IFc->send(MsgCode_GetPause, 0, 0, &retlen);

}

void test_listen(){
	printf("waiting.\n");
	rst = IFc->listen(&msg);
	if (FAILED(rst)) return;
	printf("recive msg:%x\n", msg);

	//
	// login require
	//
	if (msg == MsgCode_User_Login){

		printf("query user\n");
		int n = IFc->queryUser(&users);

		//
		// no user found, query volume list
		//
		if (n == 0) {
			printf("query volume:");
			WCHAR letter[26];
			rst = IFc->send(MsgCode_Volume_Query, letter, sizeof(letter), &retlen);
			if (FAILED(rst)) return;
			if (!retlen){ printf("no volume\n"); return; }

			for (ULONG i = 0; i < retlen / sizeof(WCHAR); i++){
				putwchar(letter[i]);
			}

			Msg_User_Registry reg = null;
			setWchar(reg.user.user, L"user", PM_NAME_MAX);
			setWchar(reg.user.group, L"group", PM_NAME_MAX);
			CoCreateGuid(&reg.user.uid);
			CoCreateGuid(&reg.user.gid);
			reg.letter = letter[0];
			setWchar(reg.password, L"password", PM_NAME_MAX);
			printf("registry user.");
			rst = IFc->send(MsgCode_User_Registry, &reg, sizeof(Msg_User_Registry), &retlen);
		}
		// is only one user
		else if (n == 1){
			Msg_User_Login login = null;
			login.user = users[0];
			setWchar(login.password, L"password", PM_NAME_MAX);
			rst = IFc->send(MsgCode_User_Login, &login, sizeof(Msg_User_Login), &retlen);

		}

	}
}