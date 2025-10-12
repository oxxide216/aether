#ifndef COMPILER_H
#define COMPILER_H

#include "aether-ir/ir.h"

Ir parse(Str code, char *input_file_name);

#endif // COMPILER_H
