#define SHL_LOG_H

#include <dirent.h>

#include "emscripten-main.h"
#include "io.h"
#include "aether/parser.h"
#include "aether/vm.h"
#include "aether/deserializer.h"
#include "emscripten-log.h"

static char *ems_loader_path = "dest/ems-loader.ae";

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
  Str code = read_file(ems_loader_path);
  if (code.len == (u32) -1) {
    INFO("Could not open file %s, listing current directory", ems_loader_path);
    list_dir(".");
    return 1;
  }

  i32 argc = 2;
  char *argv[] = { "aether", path };
  Intrinsics intrinsics = {0};
  Vm vm = vm_create(argc, argv, &intrinsics);

  Ir ir = parse(code, path);
  execute_block(&vm, &ir, false);

  vm_destroy(&vm);

  return vm.exit_code;
}
