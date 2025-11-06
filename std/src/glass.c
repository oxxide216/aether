#include "aether/vm.h"
#include "aether/misc.h"
#include "winx/winx.h"
#include "winx/event.h"
#include "glass/glass.h"
#include "glass/params.h"
#include "glass/math.h"

#define TYPE_BASE   0
#define TYPE_CIRCLE 1

typedef struct {
  f32 x, y;
  f32 u, v;
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
  GlassShader shader;
  GlassObject object;
  u64         vertex_hash;
} Glass;

static Str vertex_src = STR_LIT(
  "#version 330 core\n"
  "uniform vec2 u_resolution;\n"
  "layout (location = 0) in vec2 i_pos;\n"
  "layout (location = 1) in vec2 i_uv;\n"
  "layout (location = 2) in int i_type;\n"
  "out vec4 o_color;\n"
  "out vec2 o_uv;\n"
  "flat out int o_type;\n"
  "void main() {\n"
  "  gl_Position = vec4(i_pos.x / u_resolution.x * 2.0 - 1.0,\n"
  "                     1.0 - i_pos.y / u_resolution.y * 2.0,\n"
  "                     1.0, 1.0);\n"
  "  if (i_type == 0)\n"
  "    o_color = vec4(0.5, 0.0, 0.0, 1.0);\n"
  "  else if (i_type == 1)\n"
  "    o_color = vec4(0.0, 0.5, 0.0, 1.0);\n"
  "  o_uv = i_uv;\n"
  "  o_type = i_type;\n"
  "}\n");

static Str fragment_src = STR_LIT(
  "#version 330 core\n"
  "in vec4 o_color;\n"
  "in vec2 o_uv;\n"
  "flat in int o_type;\n"
  "out vec4 frag_color;\n"
  "void main() {\n"
  "  vec4 color = o_color;\n"
  "  vec2 uv = vec2(o_uv.x - 0.5, o_uv.y - 0.5);\n"
  "  if (o_type == 1)\n"
  "    color.a *= 1.0 - smoothstep(0.475, 0.5, length(uv));\n"
  "  frag_color = color;\n"
  "}\n");

static Glass glass = {0};
static bool initialized = false;

static u64 fnv_hash(u8 *data, u32 size) {
  u64 hash = 14695981039346656037u;

  for (u32 i = 0; i < size; ++i) {
    hash *= 1099511628211u;
    hash ^= data[i];
  }

  return hash;
}

bool run_intrinsic(Vm *vm) {
  Value *callback = value_stack_pop(&vm->stack);
  Value *height = value_stack_pop(&vm->stack);
  Value *width = value_stack_pop(&vm->stack);
  Value *title = value_stack_pop(&vm->stack);

  if (!initialized) {
    initialized = true;

    glass.winx = winx_init();
    glass.window = winx_init_window(&glass.winx, title->as.string,
                                    width->as._int,
                                    height->as._int,
                                    WinxGraphicsModeOpenGL,
                                    NULL);
    glass_init();

    GlassAttributes attributes = {0};
    glass_push_attribute(&attributes, GlassAttributeKindFloat, 2);
    glass_push_attribute(&attributes, GlassAttributeKindFloat, 2);
    glass_push_attribute(&attributes, GlassAttributeKindInt, 1);

    glass.shader = glass_init_shader(vertex_src,
                                     fragment_src,
                                     &attributes);

    glass.object = glass_init_object(&glass.shader);
  }

  bool is_running = true;

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
      } else if (event.kind == WinxEventKindResize) {
        resized = true;
        width = event.as.resize.width;
        height = event.as.resize.height;
      }
    }

    if (resized) {
      glass_resize(width, height);

      glass_set_param_2f(&glass.shader,
                         "u_resolution",
                         vec2((f32) width, (f32) height));
    }

    EXECUTE_FUNC(vm, &callback->as.func, NULL, false);

    u64 vertex_hash = fnv_hash((u8 *) glass.vertices.items,
                                glass.vertices.len * sizeof(Vertex));
    if (glass.vertex_hash != vertex_hash) {
      glass_put_object_data(&glass.object,
                            glass.vertices.items,
                            glass.vertices.len * sizeof(Vertex),
                            glass.indices.items,
                            glass.indices.len * sizeof(u32),
                            glass.indices.len,
                            true);

      glass.vertex_hash = vertex_hash;
    }

    glass_render_object(&glass.object, NULL, 0);
    glass.vertices.len = 0;
    glass.indices.len = 0;

    winx_draw(&glass.window);
  }

  winx_destroy_window(&glass.window);
  winx_cleanup(&glass.winx);

  return true;
}

bool window_size_intrinsic(Vm *vm) {
  if (!initialized)
    return true;

  Dict size = {0};

  Value *width = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  width->kind = ValueKindFloat;
  width->as._float = (f32) glass.window.width;
  dict_push_value_str_key(&vm->rc_arena, &size, STR_LIT("width"), width);

  Value *height = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  height->kind = ValueKindFloat;
  height->as._float = (f32) glass.window.height;
  dict_push_value_str_key(&vm->rc_arena, &size, STR_LIT("height"), height);

  value_stack_push_dict(&vm->stack, &vm->rc_arena, size);

  return true;
}

bool clear_intrinsic(Vm *vm) {
  Value *a = value_stack_pop(&vm->stack);
  Value *b = value_stack_pop(&vm->stack);
  Value *g = value_stack_pop(&vm->stack);
  Value *r = value_stack_pop(&vm->stack);

  if (!initialized)
    return true;

  glass_clear_screen(r->as._float, g->as._float,
                     b->as._float, a->as._float);

  return true;
}

void push_primitive(f32 x, f32 y, f32 width, f32 height, i32 type) {
  DA_APPEND(glass.indices, glass.vertices.len);
  DA_APPEND(glass.indices, glass.vertices.len + 1);
  DA_APPEND(glass.indices, glass.vertices.len + 2);
  DA_APPEND(glass.indices, glass.vertices.len + 2);
  DA_APPEND(glass.indices, glass.vertices.len + 1);
  DA_APPEND(glass.indices, glass.vertices.len + 3);

  Vertex vertex0 = { x, y, 0.0, 0.0, type };
  DA_APPEND(glass.vertices, vertex0);

  Vertex vertex1 = { x + width, y, 1.0, 0.0, type };
  DA_APPEND(glass.vertices, vertex1);

  Vertex vertex2 = { x, y + height, 0.0, 1.0, type };
  DA_APPEND(glass.vertices, vertex2);

  Vertex vertex3 = { x + width, y + height, 1.0, 1.0, type };
  DA_APPEND(glass.vertices, vertex3);
}

bool quad_intrinsic(Vm *vm) {
  Value *height = value_stack_pop(&vm->stack);
  Value *width = value_stack_pop(&vm->stack);
  Value *y = value_stack_pop(&vm->stack);
  Value *x = value_stack_pop(&vm->stack);

  if (!initialized)
    return true;

  push_primitive(x->as._float, y->as._float,
                 width->as._float,
                 height->as._float,
                 TYPE_BASE);

  return true;
}

bool circle_intrinsic(Vm *vm) {
  Value *radius = value_stack_pop(&vm->stack);
  Value *y = value_stack_pop(&vm->stack);
  Value *x = value_stack_pop(&vm->stack);

  if (!initialized)
    return true;

  push_primitive(x->as._float, y->as._float,
                 radius->as._float,
                 radius->as._float,
                 TYPE_CIRCLE);

  return true;
}

Intrinsic glass_intrinsics[] = {
  { STR_LIT("glass/run"), false, 4,
    { ValueKindString, ValueKindInt, ValueKindInt, ValueKindFunc },
    &run_intrinsic },
  { STR_LIT("glass/window-size"), true, 0, {}, &window_size_intrinsic },
  { STR_LIT("glass/clear"), false, 4,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &clear_intrinsic },
  { STR_LIT("glass/quad"), false, 4,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &quad_intrinsic },
  { STR_LIT("glass/circle"), false, 3,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &circle_intrinsic },
};

u32 glass_intrinsics_len = ARRAY_LEN(glass_intrinsics);
