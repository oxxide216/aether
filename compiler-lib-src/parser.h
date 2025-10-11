#ifndef PARSER_H
#define PARSER_H

#include "aether-ir/ir.h"
#include "shl_defs.h"
#include "shl_str.h"

Ir parse(Str code, char *file_path);

#endif // PARSER_H
