#include "aether/common.h"

InternStrings intern_strings = {0};

void block_append(Block *block, IrExpr *expr, Arena *arena) {
  if (!block->items) {
    block->cap = 1;
    block->items = arena_alloc(arena, sizeof(IrExpr *));
  } else if (block->cap <= block->len) {
    block->cap *= 2;

    IrExpr **new_items = arena_alloc(arena, block->cap * sizeof(IrExpr *));
    memcpy(new_items, block->items, block->len * sizeof(IrExpr *));
    block->items = new_items;
  }

  block->items[block->len++] = expr;
}

Str copy_str(Str str, Arena *arena) {
  for (u32 i = 0; i < intern_strings.len; ++i)
    if (str_eq(intern_strings.items[i], str))
      return intern_strings.items[i];

  Str copy;

  copy.len = str.len;
  copy.ptr = arena_alloc(arena, str.len);
  memcpy(copy.ptr, str.ptr, copy.len);

  DA_APPEND(intern_strings, copy);

  return copy;
}
