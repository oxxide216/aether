#include <emscripten.h>

#include "aether/aether.h"
#include "aether/misc.h"
#include "shl/shl-defs.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef Da(char *) CStrs;

AetherCtx ctx = {0};

static Arena ir_arena = {0};
static Macros macros = {0};
static FilePaths included_files = {0};
static Vm vm = {0};
static CStrs cstrs = {0};

EMSCRIPTEN_KEEPALIVE
void emscripten_create(void) {
  i32 argc = 1;
  char *argv[] = { "aether", NULL };
  Intrinsics intrinsics = {0};
  ctx = aether_init(argc, argv, true, &intrinsics);
}

static char *value_to_cstr(Value *value) {
  Str str = value_to_str(value, false, false, &vm.current_frame->arena);
  char *cstr = malloc(str.len + 1);
  memcpy(cstr, str.ptr, str.len);
  cstr[str.len] = '\0';

  DA_APPEND(cstrs, cstr);

  return cstr;
}

EMSCRIPTEN_KEEPALIVE
char *emscripten_eval_compiled(u8 *bytecode, u32 bytecode_len) {
  Value *result = aether_eval_bytecode(&ctx, bytecode, bytecode_len);

  return value_to_cstr(result);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_eval_macros(u8 *macro_bytecode, u32 macro_bytecode_len) {
  aether_eval_macros(&ctx, macro_bytecode, macro_bytecode_len);
}

EMSCRIPTEN_KEEPALIVE
char *emscripten_eval(char *code, char *file_path) {
  Str code_str = { code, strlen(code) };
  Str file_path_str = { file_path, strlen(file_path) };

  Value *result = aether_eval(&ctx, code_str, file_path_str);

  return value_to_cstr(result);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_destroy(void) {
  arena_free(&ir_arena);
  free(macros.items);
  macros = (Macros) {0};
  free(included_files.items);
  included_files = (FilePaths) {0};
  vm_destroy(&vm);

  for (u32 i = 0; i < cstrs.len; ++i)
    free(cstrs.items[i]);
  free(cstrs.items);
}
