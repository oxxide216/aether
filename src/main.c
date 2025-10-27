#include "io.h"
#include "aether/parser.h"
#include "aether/vm.h"
#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl/shl-arena.h"

static char *loader_path = "/usr/include/aether/loader.ae";

int main(i32 argc, char **argv) {
  Str code = read_file(loader_path);
  if (code.len == (u32) -1) {
    ERROR("Loader file was not found at %s\n", loader_path);
    exit(1);
  }

  Ir ir = parse(code, loader_path);
  RcArena rc_arena = {0};
  Intrinsics intrinsics = {0};
  return execute(&ir, argc, argv, &rc_arena, &intrinsics, NULL);
}
