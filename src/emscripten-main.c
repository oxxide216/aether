#include "emscripten-main.h"
#include "aether/deserializer.h"
#include "aether/vm.h"
#include "aether/misc.h"
#include "shl/shl-defs.h"

static Arena persistent_arena = {0};
static Macros macros = {0};
static FilePaths included_files = {0};
static Vm vm = {0};

EMSCRIPTEN_KEEPALIVE
void emscripten_create(void) {
  i32 argc = 1;
  char *argv[] = { "aether", NULL };
  Intrinsics intrinsics = {0};
  vm = vm_create(argc, argv, &intrinsics);
}

char *value_to_cstr(Value *value) {
  StringBuilder sb = {0};
  sb_push_value(&sb, value, 0, false, &vm);

  char *cstr = arena_alloc(&persistent_arena, sb.len + 1);
  memcpy(cstr, sb.buffer, sb.len);
  cstr[sb.len] = '\0';

  free(sb.buffer);

  return cstr;
}

EMSCRIPTEN_KEEPALIVE
char *emscripten_eval_compiled(u8 *bytecode, u32 bytecode_len) {
  Arena ir_arena = {0};
  Ir ir = deserialize(bytecode, bytecode_len,
                      &ir_arena, &persistent_arena);

  Value *result = execute_block(&vm, &ir, true);

  arena_free(&ir_arena);

  return value_to_cstr(result);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_eval_macros(u8 *macro_bytecode, u32 macro_bytecode_len) {
  Arena ir_arena = {0};
  Macros temp_macros = deserialize_macros(macro_bytecode,
                                          macro_bytecode_len,
                                          &ir_arena, &persistent_arena);

  if (macros.cap < macros.len + temp_macros.len) {
    macros.cap = macros.len + temp_macros.len;
    macros.items = realloc(macros.items, macros.cap * sizeof(Macro));
  }

  memcpy(macros.items + macros.len,
         temp_macros.items,
         temp_macros.len * sizeof(Macro));
  macros.len += temp_macros.len;

  free(temp_macros.items);
  arena_free(&ir_arena);
}

EMSCRIPTEN_KEEPALIVE
char *emscripten_eval(char *code, char *path) {
  Arena ir_arena = {0};
  Ir ir = parse_ex(STR(code, strlen(code)), path, &macros,
                   &included_files, &ir_arena, &persistent_arena);
  expand_macros_block(&ir, &macros, NULL, NULL, false, &persistent_arena);

  Value *result = execute_block(&vm, &ir, true);

  arena_free(&ir_arena);

  return value_to_cstr(result);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_destroy(void) {
  arena_free(&persistent_arena);
  free(macros.items);
  macros = (Macros) {0};
  free(included_files.items);
  included_files = (FilePaths) {0};
  vm_destroy(&vm);
}
