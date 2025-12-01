#include <dirent.h>

#define SHL_LOG_H

#include "emscripten-main.h"
#include "emscripten-log.h"
#include "io.h"
#include "aether/deserializer.h"
#include "aether/vm.h"
#include "aether/misc.h"

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

  return sb.buffer;
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
  vm_destroy(&vm);
}
