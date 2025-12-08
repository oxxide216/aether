#include <emscripten.h>

#include "aether/deserializer.h"
#include "aether/common.h"
#include "aether/vm.h"
#include "aether/misc.h"
#include "shl/shl-defs.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef Da(char *) CStrs;

extern InternStrings intern_strings;
extern CachedIrs cached_irs;
#ifndef NOSYSTEM
extern StringBuilder printf_sb;
#endif

static Macros macros = {0};
static FilePaths included_files = {0};
static Vm vm = {0};
static CStrs cstrs = {0};

EMSCRIPTEN_KEEPALIVE
void emscripten_create(void) {
  i32 argc = 1;
  char *argv[] = { "aether", NULL };
  Intrinsics intrinsics = {0};
  vm = vm_create(argc, argv, &intrinsics);
}

static char *value_to_cstr(Value *value) {
  StringBuilder sb = {0};
  sb_push_value(&sb, value, 0, false, &vm);

  char *cstr = malloc(sb.len + 1);
  memcpy(cstr, sb.buffer, sb.len);
  cstr[sb.len] = '\0';

  DA_APPEND(cstrs, cstr);

  free(sb.buffer);

  return cstr;
}

EMSCRIPTEN_KEEPALIVE
char *emscripten_eval_compiled(u8 *bytecode, u32 bytecode_len) {
  Arena ir_arena = {0};
  Ir ir = deserialize(bytecode, bytecode_len, &ir_arena, &vm.current_file_path);

  Value *result = execute_block(&vm, &ir, true);

  arena_free(&ir_arena);

  return value_to_cstr(result);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_eval_macros(u8 *macro_bytecode, u32 macro_bytecode_len) {
  Arena ir_arena = {0};
  Macros temp_macros = deserialize_macros(macro_bytecode, macro_bytecode_len, &ir_arena);

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
char *emscripten_eval(char *code, char *file_path) {
  Arena ir_arena = {0};
  Str file_path_str = { file_path, strlen(file_path) };
  Ir ir = parse_ex(STR(code, strlen(code)), file_path_str,
                   &macros, &included_files, &ir_arena);

  vm.current_file_path = file_path_str;
  expand_macros_block(&ir, &macros, NULL, NULL, false, &ir_arena, file_path_str);

  Value *result = execute_block(&vm, &ir, true);

  return value_to_cstr(result);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_destroy(void) {
  if (intern_strings.items)
    free(intern_strings.items);

  for (u32 i = 0; i < cached_irs.len; ++i)
    arena_free(&cached_irs.items[i].arena);
  if (cached_irs.items)
    free(cached_irs.items);

  free(printf_sb.buffer);

  free(macros.items);
  macros = (Macros) {0};
  free(included_files.items);
  included_files = (FilePaths) {0};
  vm_destroy(&vm);

  for (u32 i = 0; i < cstrs.len; ++i)
    free(cstrs.items[i]);
  free(cstrs.items);
}
