// FREE

#include "text.h"
#include "glass.h"
#include "io.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define GLYPHS_BUFFER_GROWTH_AMOUNT_BYTES_X 1024
#define GLYPHS_BUFFER_GROWTH_AMOUNT_BYTES_Y 128

void load_font(Fonts *fonts, char *path) {
  Font font = {0};

  Str data = read_file(path);

  if (data.len == (u32) -1)
    return;

  if (!stbtt_InitFont(&font.info, (u8 *) data.ptr, 0))
    return;

  font.texture = glass_init_texture(GlassFilteringModeLinear);

  DA_APPEND(*fonts, font);
}

static Glyph *lookup_glyph(Glyphs *glyphs, u32 _char, u32 line_height) {
  for (u32 i = 0; i < glyphs->len; ++i)
    if (glyphs->items[i]._char == _char &&
        glyphs->items[i].line_height == line_height)
      return glyphs->items + i;

  return NULL;
}

static void grow_glyphs_buffer(u8 **buffer, u32 *width, u32 *height, u32 filled,
                               u32 growth_width, u32 growth_height) {
  u32 new_width = *width;
  while (new_width < filled + growth_width)
    new_width += GLYPHS_BUFFER_GROWTH_AMOUNT_BYTES_X;

  u32 new_height = *height;
  while (new_height < filled + growth_height)
    new_height += GLYPHS_BUFFER_GROWTH_AMOUNT_BYTES_Y;

  if (new_width != *width || new_height != *height) {
    u8 *new_buffer = malloc(new_width * new_height);

    for (u32 y = 0; y < *height; ++y)
      for (u32 x = 0; x < *width; ++x)
        new_buffer[y * new_width + x] = (*buffer)[y * *width + x];

    free(*buffer);
    *buffer = new_buffer;
    *width = new_width;
    *height = new_height;
  }
}

static void render_glyph(u8 *glyphs_buffer, u32 glyphs_width, u32 filled,
                         u8 *bitmap, u32 bitmap_width, u32 bitmap_height) {
  for (u32 y = 0; y < bitmap_height; ++y)
    for (u32 x = 0; x < bitmap_width; ++x)
      glyphs_buffer[y * glyphs_width + x + filled] = bitmap[y * bitmap_width + x];
}

static Glyph *load_glyph(Font *font, u32 _char, u32 line_height, f32 scale) {
  Glyph *glyph = lookup_glyph(&font->glyphs, _char, line_height);
  if (glyph)
    return glyph;

  i32 y_offset;
  stbtt_GetCodepointBitmapBox(&font->info, _char, scale, scale, NULL,
                              &y_offset, NULL, NULL);

  i32 glyph_index = stbtt_FindGlyphIndex(&font->info, _char);

  i32 width, height;
  u8 *bitmap = stbtt_GetGlyphBitmap(&font->info, 0, scale, glyph_index,
                                    &width, &height, NULL, NULL);

  grow_glyphs_buffer(&font->glyphs_buffer, &font->glyphs_width,
                     &font->glyphs_height, font->glyphs_filled,
                     width, height);
  render_glyph(font->glyphs_buffer, font->glyphs_width,
               font->glyphs_filled, bitmap, width, height);

  free(bitmap);

  glass_put_texture_data(&font->texture, font->glyphs_buffer,
                         font->glyphs_width, font->glyphs_height,
                         GlassPixelKindSingleColor);

  Glyph new_glyph = {
    _char,
    line_height,
    { width, height },
    font->glyphs_filled,
    y_offset,
  };
  DA_APPEND(font->glyphs, new_glyph);

  font->glyphs_filled += width;

  return font->glyphs.items + font->glyphs.len - 1;
}

void render_text(f32 x, f32 y, u32 line_height, Str text,
                 Font *font, f32 *width, bool measure_only) {
  f32 scale = stbtt_ScaleForPixelHeight(&font->info, line_height);
  f32 x_offset = 0.0;

  for (u32 i = 0; i < text.len; ++i) {
    Glyph *glyph = load_glyph(font, text.ptr[i], line_height, scale);

    if (!measure_only) {
      Vec4 uv = {
        glyph->texture_offset / font->glyphs_width,
        0.0,
        (glyph->texture_offset + glyph->size.x) / font->glyphs_width,
        glyph->size.y / font->glyphs_height,
      };

      push_primitive(x + x_offset, y + glyph->y_offset + line_height,
                     glyph->size.x, glyph->size.y,
                     uv.x, uv.y, uv.z, uv.w,
                     1.0, 1.0, 1.0, 1.0,
                     font->texture.id, TYPE_TEXT);
    }

    x_offset += glyph->size.x;
  }

  if (width)
    *width = x_offset;
}
