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


PVOID aes(const uint8_t in[], PULONG len, bool isencrypt, uint32_t *key){

	ASSERT(in);
	ASSERT(len);
	ASSERT(key);

	//
	// allocate result memory
	//
	size_t ilen = *len;
	size_t size = ROUND_TO_SIZE(ilen, AES_SIZE);
	uint8_t *out = ExAllocatePoolWithTag(NonPagedPool, size, UTIL_TAG);
	if (!out){ KdPrint(("allocate memory failed. \n")); return NULL; }

	//
	// do aes
	//
	for (size_t i = 0; i < size; i += AES_SIZE){

		// encrypt 
		if (isencrypt)
			aes_encrypt(in + i, out + i, key, AES_KEY_SIZE);
		else
			aes_decrypt(in + i, out + i, key, AES_KEY_SIZE);
	}

	*len = size;
	return out;
}

PVOID encrypt(const PVOID in[], PULONG len, uint32_t *key){
	return aes((uint8_t*)in, len, true, key);
}

PVOID decrypt(const PVOID in[], PULONG len, uint32_t *key){
	return aes((uint8_t*)in, len, false, key);
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