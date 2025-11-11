#ifndef MACROS_H
#define MACROS_H

#include "aether/ir.h"
#include "arena.h"

typedef struct {
  Str     name;
  IrArgs  arg_names;
  IrBlock body;
  bool    has_unpack;
} Macro;

typedef Da(Macro) Macros;

void ir_block_append(IrBlock *block, IrExpr *expr, Arena *arena);

void expand_macros_block(IrBlock *block, Macros *macros,
                         IrArgs *arg_names, IrBlock *args,
                         bool unpack, Arena *arena);
void expand_macros(IrExpr *expr, Macros *macros,
                   IrArgs *arg_names, IrBlock *args,
                   bool unpack, Arena *arena);

#endif // MACROS_H
