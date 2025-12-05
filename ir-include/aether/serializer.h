#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "aether/ir.h"
#include "aether/macros.h"
#include "shl/shl-defs.h"

u8 *serialize(Ir *ir, u32 *size);
u8 *serialize_macros(Macros *macros, u32 *size);

#endif // SERIALIZER_H
