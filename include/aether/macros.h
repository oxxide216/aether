#ifndef MACROS_H
#define MACROS_H

#include "aether/parser.h"

void expand_macros(Expr *expr, Macros *macros,
                   Args *arg_names, Exprs *args,
                   bool unpack, Arena *arena, Str *file_path,
                   i16 row, i16 col, bool is_inlined);
void expand_macros_block(Exprs *block, Macros *macros,
                         Args *arg_names, Exprs *args,
                         bool unpack, Arena *arena, Str *file_path,
                         i16 row, i16 col, bool is_inlined);

#endif // MACROS_H
