#pragma once
#include <stdbool.h>
#include <stdint.h>

#include <wdm.h>

#include "aes.h"
#include "sha256.h"

#define HASH_SIZE SHA256_BLOCK_SIZE

#define UTIL_TAG 'utag'

#define AES_SIZE	AES_BLOCK_SIZE
#define AES_KEY		"1A688A2A-3AAA-4779-8B26-5AC98A976E65"
#define AES_KEY_DATA_SIZE	60
#define AES_KEY_SIZE		256


struct _IUtil
{
	NTSTATUS(*getConfig)(HANDLE hand, PWCHAR _name, PVOID * _value, PULONG _len);
	NTSTATUS(*setConfig)(HANDLE hand, PWCHAR _name, PVOID _value, ULONG _len, ULONG type);

	void(*hash)(const uint8_t data[], size_t len, uint8_t hash[]);

	PVOID(*encrypt)(const PVOID in[], PULONG len);
	PVOID(*decrypt)(const PVOID in[], PULONG len);

	void(*GUID)(GUID* guid);
};

extern struct _IUtil IUtil[1];

#ifndef ROUND_TO_SIZE
#define ROUND_TO_SIZE(_length, _alignment)                      \
            ((((ULONG_PTR)(_length)) + ((_alignment)-1)) & ~(ULONG_PTR) ((_alignment) - 1))
#endif