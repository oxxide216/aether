#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include "aether/bytecode.h"
#include "aether/parser.h"
#include "aether/arena.h"

Ir     deserialize(u8 *data, u32 size, Arena *arena, Str *file_path);
Macros deserialize_macros(u8 *data, u32 size,
                          FilePaths *included_files,
                          Arena *arena);

#endif // DESERIALIZER_H
