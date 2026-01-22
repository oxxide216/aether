#include "aether/common.h"

InternStrings intern_strings = {0};

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

Str get_file_dir(Str path) {
  for (u32 i = path.len; i > 0; --i)
    if (path.ptr[i - 1] == '/')
      return (Str) { path.ptr, i };

  return (Str) {0};
}
