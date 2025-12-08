#ifndef MACROS_H
#define MACROS_H

#include "aether/ir.h"
#include "aether/common.h"
#include "arena.h"

typedef struct {
  Str     name;
  IrArgs  arg_names;
  IrBlock body;
  bool    has_unpack;
} Macro;

typedef Da(Macro) Macros;

void expand_macros_block(IrBlock *block, Macros *macros,
                         IrArgs *arg_names, IrBlock *args,
                         bool unpack, Arena *arena,
                         Str file_path);
void expand_macros(IrExpr *expr, Macros *macros,
                   IrArgs *arg_names, IrBlock *args,
                   bool unpack, Arena *arena,
                   Str file_path);

#endif // MACROS_H
