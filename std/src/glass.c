#include "aether/vm.h"
#include "aether/misc.h"
#include "winx/winx.h"
#include "winx/event.h"
#include "glass/glass.h"
#include "glass/params.h"
#include "glass/math.h"
#include "glass-winx-event-to-value.h"

#define TYPE_BASE   0
#define TYPE_CIRCLE 1

typedef struct {
  f32 x, y;
  f32 r, g, b, a;
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
  Vertices    alt_vertices;
  Indices     alt_indices;
  GlassShader shader;
  GlassObject object;
} Glass;

typedef struct {
  f32 r, g, b, a;
} Color;

static Str vertex_src = STR_LIT(
  "#version 330 core\n"
  "uniform vec2 u_resolution;\n"
  "layout (location = 0) in vec2 i_pos;\n"
  "layout (location = 1) in vec4 i_color;\n"
  "layout (location = 2) in vec2 i_uv;\n"
  "layout (location = 3) in int i_type;\n"
  "out vec4 o_color;\n"
  "out vec2 o_uv;\n"
  "flat out int o_type;\n"
  "void main() {\n"
  "  gl_Position = vec4(i_pos.x / u_resolution.x * 2.0 - 1.0,\n"
  "                     1.0 - i_pos.y / u_resolution.y * 2.0,\n"
  "                     1.0, 1.0);\n"
  "  o_color = i_color;\n"
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

static Color clear_color = {0};

static u64 fnv_hash(u8 *data, u32 size) {
  u64 hash = 14695981039346656037u;

  for (u32 i = 0; i < size; ++i) {
    hash *= 1099511628211u;
    hash ^= data[i];
  }

  return hash;
}

Value *run_intrinsic(Vm *vm, Value **args) {
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

    glass.shader = glass_init_shader(vertex_src,
                                     fragment_src,
                                     &attributes);

    glass.object = glass_init_object(&glass.shader);
  }

  Value *state = args[3];
  Func event_handler = args[4]->as.func;
  Func update = args[5]->as.func;
  Func render = args[6]->as.func;

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

  bool is_running = true;
  u64 prev_hash = 0;

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
        state = execute_func(vm, args, &event_handler, NULL, true);
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

    state = execute_func(vm, &state, &update, NULL, true);
    if (vm->state != ExecStateContinue)
      break;

    execute_func(vm, &state, &render, NULL, false);
    if (vm->state != ExecStateContinue)
      break;

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
    glass_render_object(&glass.object, NULL, 0);

    winx_draw(&glass.window);
  }

  winx_destroy_window(&glass.window);
  winx_cleanup(&glass.winx);

  return value_unit(&vm->arena, &vm->values);
}

Value *window_size_intrinsic(Vm *vm, Value **args) {
  (void) args;

  if (!initialized)
    return value_unit(&vm->arena, &vm->values);

  Dict size = {0};

  Value *width = value_alloc(&vm->arena, &vm->values);
  width->kind = ValueKindFloat;
  width->as._float = (f32) glass.window.width;
  dict_push_value_str_key(&vm->arena, &size, STR_LIT("width"), width);

  Value *height = value_alloc(&vm->arena, &vm->values);
  height->kind = ValueKindFloat;
  height->as._float = (f32) glass.window.height;
  dict_push_value_str_key(&vm->arena, &size, STR_LIT("height"), height);

  return value_dict(size, &vm->arena, &vm->values);
}

Value *clear_intrinsic(Vm *vm, Value **args) {
  if (!initialized)
    return value_unit(&vm->arena, &vm->values);

  clear_color.r = args[0]->as._float;
  clear_color.g = args[1]->as._float;
  clear_color.b = args[2]->as._float;
  clear_color.a = args[3]->as._float;

  return value_unit(&vm->arena, &vm->values);
}

void push_primitive(f32 x, f32 y, f32 width, f32 height,
                    f32 r, f32 g, f32 b, f32 a, i32 type) {
  DA_APPEND(glass.indices, glass.vertices.len);
  DA_APPEND(glass.indices, glass.vertices.len + 1);
  DA_APPEND(glass.indices, glass.vertices.len + 2);
  DA_APPEND(glass.indices, glass.vertices.len + 2);
  DA_APPEND(glass.indices, glass.vertices.len + 1);
  DA_APPEND(glass.indices, glass.vertices.len + 3);

  Vertex vertex0 = { x, y, r, g, b, a, 0.0, 0.0, type };
  DA_APPEND(glass.vertices, vertex0);

  Vertex vertex1 = { x + width, y, r, g, b, a, 1.0, 0.0, type };
  DA_APPEND(glass.vertices, vertex1);

  Vertex vertex2 = { x, y + height, r, g, b, a, 0.0, 1.0, type };
  DA_APPEND(glass.vertices, vertex2);

  Vertex vertex3 = { x + width, y + height, r, g, b, a, 1.0, 1.0, type };
  DA_APPEND(glass.vertices, vertex3);
}

Value *quad_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  if (!initialized)
    return value_unit(&vm->arena, &vm->values);

  push_primitive(args[0]->as._float, args[1]->as._float,
                 args[2]->as._float, args[3]->as._float,
                 args[4]->as._float, args[5]->as._float,
                 args[6]->as._float, args[7]->as._float,
                 TYPE_BASE);

  return value_unit(&vm->arena, &vm->values);
}

Value *circle_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  if (!initialized)
    return value_unit(&vm->arena, &vm->values);

  push_primitive(args[0]->as._float - args[2]->as._float,
                 args[1]->as._float - args[2]->as._float,
                 args[2]->as._float * 2.0, args[2]->as._float * 2.0,
                 args[3]->as._float, args[4]->as._float,
                 args[5]->as._float, args[6]->as._float,
                 TYPE_CIRCLE);

  return value_unit(&vm->arena, &vm->values);
}

Intrinsic glass_intrinsics[] = {
  { STR_LIT("glass/run"), false, 7,
    { ValueKindString, ValueKindInt, ValueKindInt, ValueKindUnit,
      ValueKindFunc, ValueKindFunc, ValueKindFunc },
    &run_intrinsic },
  { STR_LIT("glass/window-size"), true, 0, {}, &window_size_intrinsic },
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
};

u32 glass_intrinsics_len = ARRAY_LEN(glass_intrinsics);
