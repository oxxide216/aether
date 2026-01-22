#ifndef PARSER_H
#define PARSER_H

#include "aether/bytecode.h"
#include "aether/arena.h"

typedef struct {
  Str   name;
  Args  arg_names;
  Exprs body;
  bool  has_unpack;
  u32   row, col;
} Macro;

typedef Da(Macro) Macros;

typedef struct {
  u64       code_hash;
  Exprs     ast;
  Macros    macros;
  FilePaths included_files;
  Arena     arena;
} CachedAST;

typedef Da(CachedAST) CachedASTs;

Exprs parse_ex(Str code, Str *file_path, Macros *macros,
               FilePaths *included_files, IncludePaths *include_paths,
               CachedASTs *cached_asts, Arena *arena, bool use_macros,
               u32 *root_label_index);
Ir parse(Str code, Str *file_path, CachedASTs *cached_asts);

#endif // PARSER_H
