#define SHL_LOG_H

#include <dirent.h>

#include "emscripten-main.h"
#include "io.h"
#include "aether/deserializer.h"
#include "aether/vm.h"
#include "emscripten-log.h"

static char *loader_path = "dest/loader.abc";

static Arena persistent_arena = {0};
static Macros macros = {0};
static FilePaths included_files = {0};
static Vm vm = {0};

Value *eval_compiled_intrinsic(Vm *vm, Value **args);
Value *eval_intrinsic(Vm *vm, Value **args);

static void list_dir(char *path) {
  DIR *dir;
  struct dirent *entry;
  dir = opendir(path);
  if (dir) {
    while ((entry = readdir(dir)) != NULL)
      printf("%s\n", entry->d_name);
    closedir(dir);
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_create(char *path) {
  Str bytecode = read_file(loader_path);
  if (bytecode.len == (u32) -1) {
    INFO("Could not find loader at %s, listing current directory\n", loader_path);
    list_dir(".");
    exit(1);
  }

  Arena ir_arena = {0};
  Ir ir = deserialize((u8 *) bytecode.ptr, bytecode.len,
                      &ir_arena, &persistent_arena);
  expand_macros_block(&ir, &macros, NULL, NULL, false, &persistent_arena);

  i32 argc = 4;
  char *argv[] = { "aether", "-c", "-w", path, NULL };
  Intrinsics intrinsics = {0};
  vm = vm_create(argc, argv, &intrinsics);
  execute_block(&vm, &ir, false);

  free(bytecode.ptr);
  arena_free(&ir_arena);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_eval_compiled(char *bytecode) {
  Arena ir_arena = {0};
  Ir ir = deserialize((u8 *) bytecode, strlen(bytecode),
                      &ir_arena, &persistent_arena);

  execute_block(&vm, &ir, false);

  arena_free(&ir_arena);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_eval(char *code, char *path) {
  Arena ir_arena = {0};
  Ir ir = parse_ex(STR(code, strlen(code)), path, &macros,
                   &included_files, &ir_arena, &persistent_arena);
  expand_macros_block(&ir, &macros, NULL, NULL, false, &persistent_arena);

  execute_block(&vm, &ir, false);

  arena_free(&ir_arena);
}

EMSCRIPTEN_KEEPALIVE
void emscripten_destroy(void) {
  arena_free(&persistent_arena);
  vm_destroy(&vm);
}
