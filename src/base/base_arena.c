#include "base_arena.h"
#include "base_core.h"

#include "../os/os_core.h"

#define ARENA_HEADER_SIZE sizeof(Arena)

Arena *ArenaAlloc(u64 reserve_size, u64 commit_size) {
  u64 page = OS_PageSize();
  reserve_size = AlignPow2(ARENA_HEADER_SIZE + reserve_size, page);
  commit_size = AlignPow2(ARENA_HEADER_SIZE + commit_size, page);

  void *base = OS_Reserve(reserve_size);
  OS_Commit(base, commit_size);

  Arena *arena = (Arena *)base;
  arena->current = arena;
  arena->offset = sizeof(Arena);
  arena->commited = commit_size;
  arena->reserved = reserve_size;
  return arena;
}

void ArenaRelease(Arena *arena) { OS_Release((void *)arena, arena->reserved); }

void *ArenaPushNonZero(Arena *arena, u64 size, u64 align) {
  u64 offset = AlignPow2(arena->offset, align);
  u64 end = offset + size;

  if (end > arena->reserved) {
    Assert(!"Arena out of memory");
    return 0;
  }

  if (end > arena->commited) {
    u64 page = OS_PageSize();
    u64 new_commit = AlignPow2(end, page);
    u64 commit_delta = new_commit - arena->commited;
    OS_Commit((u8 *)arena + arena->commited, commit_delta);
    arena->commited = new_commit;
  }

  arena->offset = end;

  return (u8 *)arena + offset;
}

void *ArenaPush(Arena *arena, u64 size, u64 align) {
  void *result = ArenaPushNonZero(arena, size, align);
  MemoryZero(result, size);
  return result;
}

u64 ArenaOffset(Arena *arena) { return arena->offset; }

void ArenaPopTo(Arena *arena, u64 offset) {
  if (offset < sizeof(Arena)) {
    offset = sizeof(Arena);
  }

  arena->offset = offset;
}

void ArenaPop(Arena *arena, u64 amount) {
  u64 pos_old = ArenaOffset(arena);
  u64 pos_new = pos_old;
  if (amount < pos_old) {
    pos_new = pos_old - amount;
  }
  ArenaPopTo(arena, pos_new);
}

void ArenaClear(Arena *arena) { ArenaPopTo(arena, sizeof(Arena)); }

Temp TempBegin(Arena *arena) {
  Temp temp = {0};
  temp.arena = arena;
  temp.original_offset = ArenaOffset(arena);
  return temp;
}

void TempEnd(Temp temp) { ArenaPopTo(temp.arena, temp.original_offset); }
