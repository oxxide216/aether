#ifndef COMPILER_H
#define COMPILER_H

#include "aether/ir.h"
#include "arena.h"
#include "macros.h"

typedef struct {
  u64        code_hash;
  Ir         ir;
  Macros     macros;
  FilePaths  included_files;
  Arena      arena;
} CachedIr;

typedef Da(CachedIr) CachedIrs;

Ir parse_ex(Str code, Str *file_path, Macros *macros,
            FilePaths *included_files, CachedIrs *cached_irs,
            Arena *arena, bool use_macros);
Ir parse(Str code, Str *file_path, CachedIrs *cached_irs);

#endif // COMPILER_H
