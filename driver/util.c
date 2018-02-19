#include "util.h"




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
		if (info->Type == REG_DWORD){
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

NTSTATUS setConfig(HANDLE hand, PWCHAR _name, PVOID * _value, PULONG _len){

	ASSERT(hand);
	ASSERT(_name);
	ASSERT(_value);
	ASSERT(_len);

	return STATUS_ACCESS_DENIED;
}

struct _IUtil IUtil[1] = {
	getConfig,
	setConfig,
};