#ifndef AETHER_GLASS_H
#define AETHER_GLASS_H

#include "shl/shl-defs.h"

#define TYPE_BASE    0
#define TYPE_CIRCLE  1
#define TYPE_TEXTURE 2
#define TYPE_TEXT    3

void push_primitive(f32 x, f32 y, f32 width, f32 height,
                    f32 r, f32 g, f32 b, f32 a,
                    f32 u0, f32 v0, f32 u1, f32 v1,
                    i32 texture_id, i32 type);

#endif // AETHER_GLASS_H
