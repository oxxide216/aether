#ifndef COMMON_H
#define COMMON_H

#include "arena.h"
#include "shl/shl-str.h"

typedef Da(Str *) FilePaths;
typedef Da(Str) InternStrings;
typedef Da(Str) IncludePaths;

typedef Da(Str) Args;

Str copy_str(Str str, Arena *arena);
Str get_file_dir(Str path);

#endif // COMMON_H
