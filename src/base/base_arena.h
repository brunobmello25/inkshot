#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "base_core.h"

typedef struct Arena Arena;
struct Arena {
  Arena *current;
  u64 offset;
  u64 commited;
  u64 reserved;
};

Arena *ArenaAlloc(u64 reserve_size, u64 commit_size);
void ArenaRelease(Arena *arena);
void *ArenaPush(Arena *arena, u64 size, u64 align);
void *ArenaPushNonZero(Arena *arena, u64 size, u64 align);
u64 ArenaOffset(Arena *arena);
void ArenaPopTo(Arena *arena, u64 offset);
void ArenaPop(Arena *arena, u64 amount);
void ArenaClear(Arena *arena);

typedef struct Temp Temp;
struct Temp {
  Arena *arena;
  u64 original_offset;
};

Temp TempBegin(Arena *arena);
void TempEnd(Temp temp);

#define PushArray(a, T, c) (T *)ArenaPush(a, sizeof(T) * (c), AlignOf(T))
#define PushArrayNonZero(a, T, c)                                              \
  (T *)ArenaPushNonZero(a, sizeof(T) * (c), AlignOf(T))

#define PushStruct(a, T) (T *)ArenaPush(a, sizeof(T), AlignOf(T))

#endif
