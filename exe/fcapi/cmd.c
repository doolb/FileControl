#define FC_STATIC
#include "fcapi.h"
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

void __cdecl main(){

	ULONG retlen = 0;
	HRESULT rst = S_OK;
	PUser users = NULL;

	MsgCode msg = MsgCode_Null;

	try{
		if (!IFc->open(true)) leave;

		printf("waiting.\n");
		rst = IFc->listen(&msg);
		if (FAILED(rst)) leave;
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
				if (FAILED(rst)) leave;
				if (!retlen){ printf("no volume\n"); leave; }

				for (ULONG i = 0; i < retlen / sizeof(WCHAR); i++){
					putwchar(letter[i]);
				}

				Msg_User_Registry reg = null;
				setWchar(reg.name, L"user", PM_NAME_MAX);
				setWchar(reg.group, L"group", PM_NAME_MAX);
				reg.letter = letter[0];
				setWchar(reg.password, L"password", PM_NAME_MAX);
				printf("registry user.");
				rst = IFc->send(MsgCode_User_Registry, &reg, sizeof(Msg_User_Registry), &retlen);
			}
			// is only one user
			else if (n == 1){
				Msg_User_Login login = null;
				login.user = users[0];
				setWchar(login.password, L"passwaord", PM_NAME_MAX);
				rst = IFc->send(MsgCode_User_Login, &login, sizeof(Msg_User_Login), &retlen);

			}

		}
	}
	finally{	
		if (users) free(users);

		IFc->close();
	}


	getchar();
}