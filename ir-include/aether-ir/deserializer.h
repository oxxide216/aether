#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include "aether-ir/ir.h"
#include "aether-ir/rc-arena.h"

Ir deserialize(u8 *data, u32 size, RcArena *rc_arena);

#endif // DESERIALIZER_H
