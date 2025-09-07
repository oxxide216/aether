#ifndef RC_ARENA_H
#define RC_ARENA_H

#include "shl_defs.h"

#define RC_ARENA_REGION_CAP 256

typedef struct Region Region;

struct Region {
  u8      data[RC_ARENA_REGION_CAP];
  u32     refs_count, size;
  Region *next;
};

typedef struct {
  Region *regions;
} RcArena;

void *rc_arena_alloc(RcArena *arena, u32 size);
void  rc_arena_free(RcArena *arena, void *ptr);

#endif // RC_ARENA_H
