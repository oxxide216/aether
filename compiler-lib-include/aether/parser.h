#ifndef COMPILER_H
#define COMPILER_H

#include "aether/ir.h"
#include "arena.h"
#include "macros.h"

typedef Da(Str) FilePaths;

typedef struct {
  char   *path;
  Ir      ir;
  Macros  macros;
  Arena   arena;
} CachedIr;

typedef Da(CachedIr) CachedIrs;

Ir parse_ex(Str code, char *file_path, Macros *macros,
            FilePaths *included_files, Arena *arena);
Ir parse(Str code, char *file_path);

#endif // COMPILER_H
