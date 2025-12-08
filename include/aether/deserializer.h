#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include "aether/ir.h"
#include "aether/macros.h"
#include "arena.h"

Ir     deserialize(u8 *data, u32 size, Arena *arena, Str *file_path);
Macros deserialize_macros(u8 *data, u32 size, Arena *arena);

#endif // DESERIALIZER_H
