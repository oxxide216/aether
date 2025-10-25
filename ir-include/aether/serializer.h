#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "aether/ir.h"
#include "shl/shl-defs.h"

u8 *serialize(Ir *ir, u32 *size);

#endif // SERIALIZER_H
