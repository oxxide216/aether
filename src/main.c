#include "io.h"
#include "aether/parser.h"
#include "aether/vm.h"
#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

static char *loader_paths[] = {
  "/usr/include/aether/load/loader.ae",
  "ae-src/loader.ae",
};

i32 main(i32 argc, char **argv) {
  Str code = { NULL, (u32) -1 };
  char *path_found = NULL;

  for (u32 i = 0; i < ARRAY_LEN(loader_paths); ++i) {
    code = read_file(loader_paths[i]);

    if (code.len != (u32) -1) {
      path_found = loader_paths[i];
      break;
    }
  }


  if (code.len == (u32) -1) {
    ERROR("Loader file was not found\n");
    exit(1);
  }

  Arena ir_arena = {0};
  Arena persistent_arena = {0};
  Ir ir = parse(code, path_found, &ir_arena, &persistent_arena);

  Intrinsics intrinsics = {0};
  Vm vm = vm_create(argc, argv, &intrinsics);
  execute_block(&vm, &ir, false);

  free(code.ptr);
  arena_free(&ir_arena);
  arena_free(&persistent_arena);
  vm_destroy(&vm);

  return vm.exit_code;
}
