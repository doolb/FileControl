#pragma once
#include <stdbool.h>
#include <stdint.h>

#include <wdm.h>

#include "aes.h"
#include "sha256.h"

#define HASH_SIZE SHA256_BLOCK_SIZE

#define UTIL_TAG 'utag'

struct _IUtil
{
	NTSTATUS(*getConfig)(HANDLE hand, PWCHAR _name, PVOID * _value, PULONG _len);
	NTSTATUS(*setConfig)(HANDLE hand, PWCHAR _name, PVOID * _value, PULONG _len);
};

extern struct _IUtil IUtil[1];