#ifndef COMMON_H
#define COMMON_H

#include "aether/ir.h"
#include "arena.h"

typedef Da(Str *) FilePaths;
typedef Da(Str) InternStrings;
typedef Da(Str) IncludePaths;

typedef Da(Str) Args;
typedef Da(IrExpr *) Block;
typedef Da(IrElif) Elifs;
typedef Da(IrField) Fields;
typedef Da(IrCase) Cases;

void block_append(Block *block, IrExpr *expr, Arena *arena);
Str  copy_str(Str str, Arena *arena);
Str  get_file_dir(Str path);

#endif // COMMON_H
