#ifndef COMPILER_H
#define COMPILER_H

#include "aether/ir.h"
#include "macros.h"

typedef Da(Str) FilePaths;

Ir parse_ex(Str code, char *file_path, Macros *macros,
            FilePaths *included_files);
Ir parse(Str code, char *file_path);

#endif // COMPILER_H
