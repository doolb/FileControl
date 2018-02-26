#define FC_STATIC
#include "fcapi.h"
#include <stdio.h>
#include <conio.h>

void __cdecl main(){

	ULONG retlen = 0;

	if (IFc->open(true)){
		IFc->send(MsgCode_Null, NULL, 0, &retlen);

		MsgCode msg;
		printf("waiting.\n");
		IFc->listen(&msg);
		printf("recive msg:%x\n", msg);

		if (msg == MsgCode_User_Login){
			PUser users;
			int n = IFc->queryUser(&users);
			if (n == 0) printf("no user avaible.");
		}

		IFc->close();
	}

	_getch();
}