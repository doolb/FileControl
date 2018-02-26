#define FC_STATIC
#include "fcapi.h"
#include <stdio.h>
#include <conio.h>

void __cdecl main(){

	if (IFc->open(true)){
		Msg msg = { 0 };
		msg.code = MsgCode_User_Get;
		IFc->send(&msg);

		printf("waiting.\n");
		IFc->listen(&msg.code);
		printf("recive msg:%x\n", msg.code);
		IFc->close();
	}

	_getch();
}