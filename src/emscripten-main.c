#define SHL_LOG_H

#include <dirent.h>

#include "emscripten-main.h"
#include "io.h"
#include "aether/deserializer.h"
#include "aether/vm.h"
#include "emscripten-log.h"

static char *loader_path = "dest/loader.abc";

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
i32 emscripten_main(char *path) {
  Str bytecode = read_file(loader_path);
  if (bytecode.len == (u32) -1) {
    INFO("Loader file was not found at %s, listing current directory\n", loader_path);
    list_dir(".");
    return 1;
  }

  Arena ir_arena = {0};
  Arena persistent_arena = {0};
  Ir ir = deserialize((u8 *) bytecode.ptr, bytecode.len, &ir_arena, &persistent_arena);

  i32 argc = 4;
  char *argv[] = { "aether", "-c", "-w", path, NULL };
  Intrinsics intrinsics = {0};
  Vm vm = vm_create(argc, argv, &intrinsics);
  execute_block(&vm, &ir, false);

  free(bytecode.ptr);
  arena_free(&ir_arena);
  arena_free(&persistent_arena);
  vm_destroy(&vm);

  return vm.exit_code;
}
