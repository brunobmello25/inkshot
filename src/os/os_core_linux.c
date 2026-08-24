#include "os_core.h"

#include <sys/mman.h>
#include <unistd.h>

u64 OS_PageSize(void) { return (u64)sysconf(_SC_PAGESIZE); }

OS_Handle OS_HandleFromPointer(void *p) {
  OS_Handle result = {0};
  result.u64[0] = (u64)p;
  return result;
}

void *OS_Reserve(u64 bytes) {
  u64 page = OS_PageSize();
  bytes = AlignPow2(bytes, page);
  void *ptr = mmap(0, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED)
    return 0;
  return ptr;
}

void OS_Commit(void *ptr, u64 bytes) {
  u64 page = OS_PageSize();
  u64 offset = (u64)ptr % page;
  u64 aligned_ptr = (u64)ptr - offset;
  u64 aligned_bytes = bytes + offset;
  aligned_bytes = AlignPow2(aligned_bytes, page);
  mprotect((void *)aligned_ptr, aligned_bytes, PROT_READ | PROT_WRITE);
}

void OS_Decommit(void *ptr, u64 bytes) {
  u64 page = OS_PageSize();
  u64 offset = (u64)ptr % page;
  u64 aligned_ptr = (u64)ptr - offset;
  u64 aligned_bytes = bytes + offset;
  aligned_bytes = AlignPow2(aligned_bytes, page);
  mprotect((void *)aligned_ptr, aligned_bytes, PROT_NONE);
}

void OS_Release(void *ptr, u64 bytes) {
  u64 page = OS_PageSize();
  bytes = AlignPow2(bytes, page);
  munmap(ptr, bytes);
}
