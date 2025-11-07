#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include "aether/ir.h"
#include "arena.h"

Ir deserialize(u8 *data, u32 size, Arena *arena, Arena *str_arena);

#endif // DESERIALIZER_H
