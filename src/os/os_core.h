#ifndef OS_CORE_H
#define OS_CORE_H

#include "../base/base_core.h"

typedef struct OS_Handle OS_Handle;
struct OS_Handle {
  u64 u64[1];
};

u64 OS_PageSize(void);
void *OS_Reserve(u64 bytes);
void OS_Commit(void *ptr, u64 bytes);
void OS_Decommit(void *ptr, u64 bytes);
void OS_Release(void *ptr, u64 bytes);

#endif
