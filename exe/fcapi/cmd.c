#define FC_STATIC
#include "fcapi.h"
#include <stdio.h>
#include <conio.h>

#define N 129

void __cdecl main(){

	WCHAR in[N] = { 0 };
	WCHAR out[N] = { 0 };

	if (fc_open()){
		printf("input message:");
		scanf_s("%ws", in, N);
		if (fc_send(in, N, out, N)){
			printf("return:%ws", out);
		}
		fc_close();
		_getch();
	}
}