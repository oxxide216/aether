#ifndef COMMON_H
#define COMMON_H

#include "arena.h"
#include "shl/shl-str.h"

typedef Da(Str *) FilePaths;
typedef Da(Str) InternStrings;
typedef Da(Str) IncludePaths;

typedef Da(u16) Args;

u16 copy_str(Str str);
Str get_str(u16 id);
Str get_file_dir(Str path);

#endif // COMMON_H
