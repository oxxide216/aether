#include <stdio.h>

#include "aether-ir/rc-arena.h"

void *rc_arena_alloc(RcArena *arena, u32 size) {
  u32 new_cap = RC_ARENA_REGION_CAP;
  if (size > RC_ARENA_REGION_CAP)
    new_cap = size;

  Region *prev_region = NULL;
  Region *region = arena->regions;
  while (region) {
    if (region->len + size <= region->cap) {
      ++region->refs_count;
      void *ptr = region->data + region->len;
      region->len += size;
      return ptr;
    }

    prev_region = region;
    region = region->next;
  }

  Region *new_region = malloc(sizeof(Region) + new_cap);
  *new_region = (Region) {0};
  new_region->data = new_region + sizeof(Region);
  new_region->cap = new_cap;
  new_region->len = size;
  new_region->refs_count = 1;
  new_region->next = region;

  if (prev_region)
    prev_region->next = new_region;
  else
    arena->regions = new_region;

  return new_region->data;
}

void *rc_arena_clone(RcArena *arena, void *data) {
  Region *region = arena->regions;
  while (region) {
    if (region->refs_count > 0 &&
        data >= region->data &&
        data < region->data + region->cap) {
      ++region->refs_count;

      break;
    }

    region = region->next;
  }

  return data;
}

void rc_arena_free(RcArena *arena, void *data) {
  Region *prev_region = arena->regions;
  Region *region = arena->regions;
  while (region) {
    if (region->refs_count > 0 &&
        data >= region->data &&
        data < region->data + region->cap) {
      --region->refs_count;

      if (region->refs_count == 0) {
        if (prev_region)
          prev_region->next = region->next;
        else
          arena->regions = region->next;

        free(region);
      }

      return;
    }

    prev_region = region;
    region = region->next;
  }
}
