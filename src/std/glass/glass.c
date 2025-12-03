#include "glass.h"
#include "aether/vm.h"
#include "aether/misc.h"
#include "winx/winx.h"
#include "glass/glass.h"
#include "glass/params.h"
#include "glass/math.h"
#include "winx/event.h"
#include "winx-event-to-value.h"
#include "text.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_TEXTURES_SUPPORTED     32
#define MAX_TEXTURES_SUPPORTED_STR "32"
#define MAIN_LOOP_FRAME_LENGTH     100

typedef struct {
  f32 x, y;
  f32 r, g, b, a;
  f32 u, v;
  i32 texture_id;
  i32 type;
} Vertex;

typedef Da(Vertex) Vertices;
typedef Da(u32) Indices;
typedef Da(WinxEvent) Events;

typedef struct {
  Winx        winx;
  WinxWindow  window;
  Events      events;
  u32         width, height;
  Vertices    vertices;
  Indices     indices;
  Vertices    alt_vertices;
  Indices     alt_indices;
  GlassShader shader;
  GlassObject object;
  i32         texture_ids[MAX_TEXTURES_SUPPORTED];
  u32         textures_loaded;
  Fonts       fonts;
} Glass;

typedef struct {
  f32 r, g, b, a;
} Color;

static Str vertex_src = STR_LIT(
  "#version 400 core\n"
  "uniform vec2 u_resolution;\n"
  "layout (location = 0) in vec2 i_pos;\n"
  "layout (location = 1) in vec4 i_color;\n"
  "layout (location = 2) in vec2 i_uv;\n"
  "layout (location = 3) in int i_texture_index;\n"
  "layout (location = 4) in int i_type;\n"
  "out vec4 o_color;\n"
  "out vec2 o_uv;\n"
  "flat out int o_texture_index;\n"
  "flat out int o_type;\n"
  "void main(void) {\n"
  "  gl_Position = vec4(i_pos.x / u_resolution.x * 2.0 - 1.0,\n"
  "                     1.0 - i_pos.y / u_resolution.y * 2.0,\n"
  "                     1.0, 1.0);\n"
  "  o_color = i_color;\n"
  "  o_uv = i_uv;\n"
  "  o_texture_index = i_texture_index;\n"
  "  o_type = i_type;\n"
  "}\n");

static Str fragment_src = STR_LIT(
  "#version 400 core\n"
  "uniform sampler2D u_textures[" MAX_TEXTURES_SUPPORTED_STR "];\n"
  "in vec4 o_color;\n"
  "in vec2 o_uv;\n"
  "flat in int o_texture_index;\n"
  "flat in int o_type;\n"
  "out vec4 frag_color;\n"
  "void main(void ) {\n"
  "  vec4 color = o_color;\n"
  "  vec2 uv = vec2(o_uv.x - 0.5, o_uv.y - 0.5);\n"
  "  if (o_type == 1)\n"
  "    color.a *= 1.0 - smoothstep(0.475, 0.5, length(uv));\n"
  "  else if (o_type == 3)\n"
  "    color.a *= texture(u_textures[o_texture_index], o_uv).r;\n"
  "  if (o_type == 2)\n"
  "    frag_color = o_color * texture(u_textures[o_texture_index], o_uv);\n"
  "  else\n"
  "    frag_color = color;\n"
  "}\n");

static i32 samplers[MAX_TEXTURES_SUPPORTED];

static Glass glass = {0};
static bool initialized = false;

static Color clear_color = {0};

static Value *try_get_dict_field_of_kind_str_key(Dict *dict, Str key,
                                                 ValueKind expected_kind,
                                                 char *intrinsic_name,
                                                 char *arg_name) {
  Value *result = NULL;

  for (u32 i = 0; i < dict->len; ++i)
    if (dict->items[i].key->kind == ValueKindString &&
        str_eq(dict->items[i].key->as.string, key))
      result = dict->items[i].value;

  if (!result) {
    ERROR("%s: %s should have a "STR_FMT" field\n",
          intrinsic_name, arg_name, STR_ARG(key));
  } else if (result->kind != expected_kind) {
    ERROR(""STR_FMT" field of %s has unexpected type\n",
          STR_ARG(key), arg_name);

    return NULL;
  }

  return result;
}

Value *load_texture_intrinsic(Vm *vm, Value **args) {
  Str file_name = args[0]->as.string;
  char *file_name_cstr = malloc(file_name.len + 1);
  memcpy(file_name_cstr, file_name.ptr, file_name.len);
  file_name_cstr[file_name.len] = '\0';

  i32 width = 0;
  i32 height = 0;
  u8 *buffer = stbi_load(file_name_cstr, &width, &height, NULL, 4);

  free(file_name_cstr);

  if (!buffer) {
    stbi_image_free(buffer);

    return value_unit(vm->current_frame);
  }

  GlassTexture texture = glass_init_texture(GlassFilteringModeLinear);

  if (texture.id == 0) {
    stbi_image_free(buffer);

    return value_unit(vm->current_frame);
  }

  glass_put_texture_data(&texture, buffer, width, height, GlassPixelKindRGBA);

  stbi_image_free(buffer);

  Dict result = {0};

  dict_push_value_str_key(vm->current_frame, &result, STR_LIT("id"),
                          value_int(texture.id, vm->current_frame));
  dict_push_value_str_key(vm->current_frame, &result, STR_LIT("width"),
                          value_float((f64) width, vm->current_frame));
  dict_push_value_str_key(vm->current_frame, &result, STR_LIT("height"),
                          value_float((f64) height, vm->current_frame));

  return value_dict(result, vm->current_frame);
}

Value *load_font_intrinsic(Vm *vm, Value **args) {
  u32 index = glass.fonts.len;

  Str file_name = args[0]->as.string;
  char *file_name_cstr = malloc(file_name.len + 1);
  memcpy(file_name_cstr, file_name.ptr, file_name.len);
  file_name_cstr[file_name.len] = '\0';

  load_font(&glass.fonts, file_name_cstr);

  free(file_name_cstr);

  if (glass.fonts.len == index)
    return value_unit(vm->current_frame);

  Dict result = {0};

  dict_push_value_str_key(vm->current_frame, &result, STR_LIT("id"),
                          value_int(index, vm->current_frame));

  return value_dict(result, vm->current_frame);
}

static u64 fnv_hash(u8 *data, u32 size) {
  u64 hash = (u64) 14695981039346656037u;

  for (u32 i = 0; i < size; ++i) {
    hash *= 1099511628211u;
    hash ^= data[i];
  }

  return hash;
}

Value *run_intrinsic(Vm *vm, Value **args) {
  Func init = args[3]->as.func;
  Func event_handler = args[4]->as.func;
  Func update = args[5]->as.func;
  Func render = args[6]->as.func;

  if (init.args.len != 0) {
    ERROR("glass/run: init should have zero arguments\n");
    vm->state = ExecStateExit;
    return NULL;
  }

  if (event_handler.args.len != 2) {
    ERROR("glass/run: event handler should have two arguments\n");
    vm->state = ExecStateExit;
    return NULL;
  }

  if (update.args.len != 1) {
    ERROR("glass/run: update body should have one argument\n");
    vm->state = ExecStateExit;
    return NULL;
  }

  if (render.args.len != 1) {
    ERROR("glass/run: render body should have one argument\n");
    vm->state = ExecStateExit;
    return NULL;
  }

  if (!initialized) {
    initialized = true;

    glass.winx = winx_init();
    glass.window = winx_init_window(&glass.winx, args[0]->as.string,
                                    args[1]->as._int,
                                    args[2]->as._int,
                                    WinxGraphicsModeOpenGL,
                                    NULL);
    glass_init();

    GlassAttributes attributes = {0};
    glass_push_attribute(&attributes, GlassAttributeKindFloat, 2);
    glass_push_attribute(&attributes, GlassAttributeKindFloat, 4);
    glass_push_attribute(&attributes, GlassAttributeKindFloat, 2);
    glass_push_attribute(&attributes, GlassAttributeKindInt, 1);
    glass_push_attribute(&attributes, GlassAttributeKindInt, 1);

    glass.shader = glass_init_shader(vertex_src,
                                     fragment_src,
                                     &attributes);

    glass.object = glass_init_object(&glass.shader);

    for (i32 i = 0; i < MAX_TEXTURES_SUPPORTED; ++i)
      samplers[i] = i;

    glass_set_param_1i_array(&glass.shader, "u_textures",
                             samplers, MAX_TEXTURES_SUPPORTED);
  }

  begin_frame(vm);
  vm->current_frame->can_lookup_through = true;

  begin_frame(vm);
  vm->current_frame->can_lookup_through = true;

  Value *state = execute_func(vm, &state, &init, NULL, true);
  if (vm->state != ExecStateContinue) {
    winx_destroy_window(&glass.window);
    winx_cleanup(&glass.winx);

    vm->state = ExecStateExit;
    return NULL;
  }

  bool is_running = true;
  u64 prev_hash = 0;
  u32 i = 0;

  while (is_running) {
    bool resized = false;
    u32 width = 0;
    u32 height = 0;

    glass.events.len = 0;
    WinxEvent event;
    while ((event = winx_get_event(&glass.window, false)).kind != WinxEventKindNone) {
      DA_APPEND(glass.events, event);

      if (event.kind == WinxEventKindQuit) {
        is_running = false;
      } else {
        Value *event_value = winx_event_to_value(&event, vm);
        Value *args[2] = { state, event_value };
        execute_func(vm, args, &event_handler, NULL, false);
        if (vm->state != ExecStateContinue)
          break;

        if (event.kind == WinxEventKindResize) {
          resized = true;
          width = event.as.resize.width;
          height = event.as.resize.height;
        }
      }
    }

    if (resized) {
      glass_resize(width, height);

      glass_set_param_2f(&glass.shader,
                         "u_resolution",
                         vec2((f32) width, (f32) height));
    }

    execute_func(vm, &state, &update, NULL, false);
    if (vm->state != ExecStateContinue)
      break;

    execute_func(vm, &state, &render, NULL, false);
    if (vm->state != ExecStateContinue)
      break;

    if (i++ == MAIN_LOOP_FRAME_LENGTH) {
      StackFrame *prev_frame = vm->current_frame->prev;

      state = value_clone(state, prev_frame->prev);

      end_frame(vm);
      begin_frame(vm);

      StackFrame temp_frame = *prev_frame;
      memcpy(prev_frame, vm->current_frame, offsetof(StackFrame, next));
      memcpy(vm->current_frame, &temp_frame, offsetof(StackFrame, next));

      i = 0;
    }

    u64 new_hash = fnv_hash((u8 *) glass.vertices.items,
                            glass.vertices.len * sizeof(Vertex));
    if (prev_hash != new_hash) {
      prev_hash = new_hash;

      glass_put_object_data(&glass.object,
                            glass.vertices.items,
                            glass.vertices.len * sizeof(Vertex),
                            glass.indices.items,
                            glass.indices.len * sizeof(u32),
                            glass.indices.len,
                            true);
    }

    glass.vertices.len = 0;
    glass.indices.len = 0;

    glass_clear_screen(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glass_render_object_raw(&glass.object, glass.texture_ids, MAX_TEXTURES_SUPPORTED);

    winx_draw(&glass.window);
  }

  vm->current_frame->can_lookup_through = false;
  end_frame(vm);

  vm->current_frame->can_lookup_through = false;
  end_frame(vm);

  winx_destroy_window(&glass.window);
  winx_cleanup(&glass.winx);

  initialized = false;

  return value_unit(vm->current_frame);
}

Value *window_size_intrinsic(Vm *vm, Value **args) {
  (void) args;

  if (!initialized)
    return value_unit(vm->current_frame);

  Dict size = {0};

  Value *width = value_alloc(vm->current_frame);
  width->kind = ValueKindFloat;
  width->as._float = (f32) glass.window.width;
  dict_push_value_str_key(vm->current_frame, &size, STR_LIT("width"), width);

  Value *height = value_alloc(vm->current_frame);
  height->kind = ValueKindFloat;
  height->as._float = (f32) glass.window.height;
  dict_push_value_str_key(vm->current_frame, &size, STR_LIT("height"), height);

  return value_dict(size, vm->current_frame);
}

Value *text_width_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(vm->current_frame);

  Value *id = try_get_dict_field_of_kind_str_key(&args[1]->as.dict, STR_LIT("id"),
                                                 ValueKindInt, "font", "font");
  if (!id)
    vm->state = ExecStateExit;

  if (id->as._int < glass.fonts.len) {
    Font *font = glass.fonts.items + id->as._int;

    f32 result = 0.0;
    render_text(0.0, 0.0, args[0]->as._int, args[2]->as.string, font, &result, true);

    return value_float(result, vm->current_frame);
  }

  return value_unit(vm->current_frame);
}

Value *clear_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(vm->current_frame);

  clear_color.r = args[0]->as._float;
  clear_color.g = args[1]->as._float;
  clear_color.b = args[2]->as._float;
  clear_color.a = args[3]->as._float;

  return value_unit(vm->current_frame);
}

void push_primitive(f32 x, f32 y, f32 width, f32 height,
                    f32 u0, f32 v0, f32 u1, f32 v1,
                    f32 r, f32 g, f32 b, f32 a,
                    i32 texture_id, i32 type) {
  u32 index = (u32) -1;

  if (texture_id != 0) {
    for (u32 i = 0; i < MAX_TEXTURES_SUPPORTED; ++i) {
      if (glass.texture_ids[i] == texture_id) {
        index = i;
        break;
      }
    }

    if (index == (u32) -1) {
      index = glass.textures_loaded;
      glass.textures_loaded = (glass.textures_loaded + 1) % MAX_TEXTURES_SUPPORTED;
    }

    glass.texture_ids[index] = texture_id;
  }

  DA_APPEND(glass.indices, glass.vertices.len);
  DA_APPEND(glass.indices, glass.vertices.len + 1);
  DA_APPEND(glass.indices, glass.vertices.len + 2);
  DA_APPEND(glass.indices, glass.vertices.len + 2);
  DA_APPEND(glass.indices, glass.vertices.len + 1);
  DA_APPEND(glass.indices, glass.vertices.len + 3);

  Vertex vertex0 = { x, y, r, g, b, a, u0, v0, index, type };
  DA_APPEND(glass.vertices, vertex0);

  Vertex vertex1 = { x + width, y, r, g, b, a, u1, v0, index, type };
  DA_APPEND(glass.vertices, vertex1);

  Vertex vertex2 = { x, y + height, r, g, b, a, u0, v1, index, type };
  DA_APPEND(glass.vertices, vertex2);

  Vertex vertex3 = { x + width, y + height, r, g, b, a, u1, v1, index, type };
  DA_APPEND(glass.vertices, vertex3);
}

Value *quad_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  if (!initialized)
    return value_unit(vm->current_frame);

  push_primitive(args[0]->as._float, args[1]->as._float,
                 args[2]->as._float, args[3]->as._float,
                 0.0, 0.0, 1.0, 1.0,
                 args[4]->as._float, args[5]->as._float,
                 args[6]->as._float, args[7]->as._float,
                 0, TYPE_BASE);

  return value_unit(vm->current_frame);
}

Value *circle_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  if (!initialized)
    return value_unit(vm->current_frame);

  push_primitive(args[0]->as._float - args[2]->as._float,
                 args[1]->as._float - args[2]->as._float,
                 args[2]->as._float * 2.0, args[2]->as._float * 2.0,
                 0.0, 0.0, 1.0, 1.0,
                 args[3]->as._float, args[4]->as._float,
                 args[5]->as._float, args[6]->as._float,
                 0, TYPE_CIRCLE);

  return value_unit(vm->current_frame);
}

Value *texture_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(vm->current_frame);

  Value *id = try_get_dict_field_of_kind_str_key(&args[2]->as.dict, STR_LIT("id"),
                                                 ValueKindInt, "texture", "texture");
  if (!id)
    vm->state = ExecStateExit;

  Value *width = try_get_dict_field_of_kind_str_key(&args[2]->as.dict, STR_LIT("width"),
                                                    ValueKindFloat, "texture", "texture");
  if (!width)
    vm->state = ExecStateExit;

  Value *height = try_get_dict_field_of_kind_str_key(&args[2]->as.dict, STR_LIT("height"),
                                                     ValueKindFloat, "texture", "texture");
  if (!height)
    vm->state = ExecStateExit;

  push_primitive(args[0]->as._float, args[1]->as._float,
                 width->as._float, height->as._float,
                 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                 id->as._int, TYPE_TEXTURE);

  return value_unit(vm->current_frame);
}

Value *textured_quad_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(vm->current_frame);

  Value *id = try_get_dict_field_of_kind_str_key(&args[4]->as.dict, STR_LIT("id"),
                                                 ValueKindInt, "texture", "texture");
  if (!id)
    vm->state = ExecStateExit;

  push_primitive(args[0]->as._float, args[1]->as._float,
                 args[2]->as._float, args[3]->as._float,
                 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                 id->as._int, TYPE_TEXTURE);

  return value_unit(vm->current_frame);
}

Value *tile_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(vm->current_frame);

  Value *id = try_get_dict_field_of_kind_str_key(&args[8]->as.dict, STR_LIT("id"),
                                                 ValueKindInt, "texture", "texture");
  if (!id)
    vm->state = ExecStateExit;

  push_primitive(args[0]->as._float, args[1]->as._float,
                 args[2]->as._float, args[3]->as._float,
                 args[4]->as._float, args[5]->as._float,
                 args[6]->as._float, args[7]->as._float,
                 1.0, 1.0, 1.0, 1.0,
                 id->as._int, TYPE_TEXTURE);

  return value_unit(vm->current_frame);
}

Value *text_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(vm->current_frame);

  Value *id = try_get_dict_field_of_kind_str_key(&args[3]->as.dict, STR_LIT("id"),
                                                 ValueKindInt, "font", "font");
  if (!id)
    vm->state = ExecStateExit;

  if (id->as._int < glass.fonts.len) {
    Font *font = glass.fonts.items + id->as._int;

    render_text(args[0]->as._float, args[1]->as._float, args[2]->as._int,
                args[4]->as.string, font, NULL, false);
  }

  return value_unit(vm->current_frame);
}

Intrinsic glass_intrinsics[] = {
  { STR_LIT("glass/load-texture"), true, 1, { ValueKindString }, &load_texture_intrinsic },
  { STR_LIT("glass/load-font"), true, 1, { ValueKindString }, &load_font_intrinsic },
  { STR_LIT("glass/run"), false, 7,
    { ValueKindString, ValueKindInt, ValueKindInt,
      ValueKindFunc, ValueKindFunc, ValueKindFunc, ValueKindFunc },
    &run_intrinsic },
  { STR_LIT("glass/window-size"), true, 0, {}, &window_size_intrinsic },
  { STR_LIT("glass/text-width"), true, 3,
    { ValueKindInt, ValueKindDict, ValueKindString },
    &text_width_intrinsic },
  { STR_LIT("glass/clear"), false, 4,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &clear_intrinsic },
  { STR_LIT("glass/quad"), false, 8,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat,
      ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &quad_intrinsic },
  { STR_LIT("glass/circle"), false, 7,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat,
      ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &circle_intrinsic },
  { STR_LIT("glass/texture"), false, 3,
    { ValueKindFloat, ValueKindFloat, ValueKindDict },
    &texture_intrinsic },
  { STR_LIT("glass/textured-quad"), false, 5,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindDict },
    &textured_quad_intrinsic },
  { STR_LIT("glass/tile"), false, 9,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat,
      ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindDict },
    &tile_intrinsic },
  { STR_LIT("glass/text"), false, 5,
    { ValueKindFloat, ValueKindFloat, ValueKindInt, ValueKindDict, ValueKindString },
    &text_intrinsic },
};

u32 glass_intrinsics_len = ARRAY_LEN(glass_intrinsics);
