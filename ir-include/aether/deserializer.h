#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include "aether/ir.h"
#include "aether/macros.h"
#include "arena.h"

Ir     deserialize(u8 *data, u32 size, Arena *arena,
                   Arena *persistent_arena);
Macros deserialize_macros(u8 *data, u32 size, Arena *arena,
                          Arena *persistent_arena);

#endif // DESERIALIZER_H
