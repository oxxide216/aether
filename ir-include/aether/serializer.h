#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "aether/ir.h"
#include "../../ir-src/shl_defs.h"

u8 *serialize(Ir *ir, u32 *size);

#endif // SERIALIZER_H
