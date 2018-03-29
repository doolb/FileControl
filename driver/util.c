#include "util.h"
#include <stdlib.h>



NTSTATUS getConfig(HANDLE hand, PWCHAR _name, PVOID * _value, PULONG _len){

	ASSERT(hand);
	ASSERT(_name);
	ASSERT(_value);
	ASSERT(_len);

	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING name;
	PUCHAR buff = NULL;
	PUCHAR rbuff = NULL;
	ULONG retlen = 0;
	PKEY_VALUE_PARTIAL_INFORMATION info;

	try{
		//
		// value name
		//
		RtlInitUnicodeString(&name, _name);

		retlen = *_len;
		//
		// if size is zero, we need detech it in runtime
		//
		if (retlen == 0){
			//
			// get the value data length
			//
			status = ZwQueryValueKey(hand, &name, KeyValuePartialInformation, NULL, 0, &retlen);
			if (retlen == 0){ KdPrint(("get value failed: %x, %wZ \n", status, &name)); leave; }

			//
			// allocate result buffer
			// 
			rbuff = ExAllocatePoolWithTag(NonPagedPool, retlen, UTIL_TAG);
			if (!rbuff){ KdPrint(("allocate memory for value failed. \n")); leave; }
		}

		//
		// allocate buffer memory
		//
		buff = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + retlen, UTIL_TAG);
		if (!buff){ KdPrint(("allocate memory for buff failed. \n")); leave; }
		info = (PKEY_VALUE_PARTIAL_INFORMATION)buff;

		//
		// now we can get the data
		//
		status = ZwQueryValueKey(hand, &name, KeyValuePartialInformation, info, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + retlen, &retlen);
		if (!NT_SUCCESS(status)){ KdPrint(("get data failed. %x:%wZ \n", status, &name)); leave; }

		//
		// save data
		//
		if (info->Type == REG_DWORD || info->Type == REG_QWORD){
			*((PULONG)(_value)) = (ULONG)info->Data[0];
		}
		else{
			memcpy_s(rbuff, info->DataLength, info->Data, info->DataLength);
			(*_value) = rbuff;
		}

		(*_len) = info->DataLength;
	}
	finally{
		if (buff) ExFreePoolWithTag(buff, UTIL_TAG); buff = NULL;
	}

	return status;
}

NTSTATUS setConfig(HANDLE hand, PWCHAR _name, PVOID _value, ULONG _len, ULONG type){

	ASSERT(hand);
	ASSERT(_name);
	ASSERT(_value);
	ASSERT(_len);

	//
	// value name
	//
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, _name);

	//
	// set value
	//
	NTSTATUS status = ZwSetValueKey(hand, &name, 0, type, _value, _len);


	return status;
}


bool aes(const uint8_t in[], uint8_t out[], ULONG len, bool isencrypt, uint32_t *key){

	ASSERT(in);
	ASSERT(out);
	ASSERT(len);
	ASSERT(key);

	// check length
	if (len <= 0 || len % AES_SIZE != 0){ return false; }

	//
	// do aes
	//
	for (size_t i = 0; i < len; i += AES_SIZE){

		// encrypt 
		if (isencrypt)
			aes_encrypt(in + i, out + i, key, AES_KEY_SIZE);
		else
			aes_decrypt(in + i, out + i, key, AES_KEY_SIZE);
	}

	return true;
}

bool encrypt(const PVOID in, PVOID out, ULONG len, uint32_t *key){
	return aes((uint8_t*)in, (uint8_t*)out, len, true, key);
}

bool decrypt(const PVOID in, PVOID out, ULONG len, uint32_t *key){
	return aes((uint8_t*)in, (uint8_t*)out, len, false, key);
}


void createGuid(GUID* guid){
	ASSERT(guid);

	guid->Data1 = rand();
	guid->Data2 = (unsigned short)rand();
	guid->Data3 = (unsigned short)rand();

	for (int i = 0; i < 8; i++)
		guid->Data4[i] = (unsigned char)rand();
}

struct _IUtil IUtil[1] = {
	getConfig,
	setConfig,

	sha256,

	encrypt,
	decrypt,

	createGuid
};