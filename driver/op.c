#include "op.h"
extern PFLT_INSTANCE gInstance;

NTSTATUS opPreCheck(PCFLT_RELATED_OBJECTS obj){
	if (obj->Instance == gInstance) return STATUS_SUCCESS;

	return STATUS_INVALID_PARAMETER;
}