#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "aether/ir.h"
#include "aether/parser.h"
#include "aether/macros.h"
#include "shl/shl-defs.h"

typedef struct {
  Str path;
  u32 offset;
} FilePathOffset;

typedef Da(FilePathOffset) FilePathOffsets;

u8 *serialize(Ir *ir, u32 *size, FilePaths *included_files);
u8 *serialize_macros(Macros *macros, u32 *size, FilePaths *included_files);

#endif // SERIALIZER_H
