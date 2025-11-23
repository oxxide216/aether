#ifndef AETHER_GLASS_TEXT_H
#define AETHER_GLASS_TEXT_H

#include "glass/glass.h"
#include "glass/math.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"
#include "stb_truetype.h"

typedef struct {
  u32  _char;
  u32  line_height;
  Vec2 size;
  f32  texture_offset;
  f32  y_offset;
} Glyph;

typedef Da(Glyph) Glyphs;

typedef struct {
  stbtt_fontinfo  info;
  GlassTexture    texture;
  Glyphs          glyphs;
  u8             *glyphs_buffer;
  u32             glyphs_width;
  u32             glyphs_height;
  u32             glyphs_filled;
} Font;

typedef Da(Font) Fonts;

void load_font(Fonts *fonts, char *path);

void render_text(f32 x, f32 y, u32 line_height, Str text,
                 Font *font, f32 *width, bool measure_only);

#endif // AETHER_GLASS_TEXT_H
