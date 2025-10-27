#ifndef MACROS_H
#define MACROS_H

#include "aether/ir.h"

typedef struct {
  Str     name;
  IrArgs  arg_names;
  IrBlock body;
  bool    has_unpack;
} Macro;

typedef Da(Macro) Macros;

void expand_macros_block(IrBlock *block, Macros *macros,
                         IrArgs *arg_names, IrBlock *args,
                         bool unpack);
void expand_macros(IrExpr *expr, Macros *macros,
                   IrArgs *arg_names, IrBlock *args,
                   bool unpack);

#endif // MACROS_H
