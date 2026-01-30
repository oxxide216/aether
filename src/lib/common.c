#include "aether/common.h"
#include "aether/misc.h"

InternStrings intern_strings = {0};
Arena intern_arena = {0};

u16 copy_str(Str str) {
  for (u16 i = 0; i < intern_strings.len; ++i)
    if (str_eq(intern_strings.items[i], str))
      return i;

  Str copy;

  copy.len = str.len;
  copy.ptr = arena_alloc(&intern_arena, str.len);
  memcpy(copy.ptr, str.ptr, copy.len);

  DA_ARENA_APPEND(intern_strings, copy, &intern_arena);

  return intern_strings.len - 1;
}

Str get_str(u16 id) {
  return intern_strings.items[id];
}

Str get_file_dir(Str path) {
  for (u32 i = path.len; i > 0; --i)
    if (path.ptr[i - 1] == '/')
      return (Str) { path.ptr, i };

  return (Str) {0};
}
