#define FC_STATIC
#include "fcapi.h"
#include <stdio.h>
#include <conio.h>

#define N 129

void __cdecl main(){
	IFc->init(true);

	if (IFc->open()){
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