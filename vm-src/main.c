#include "shl_defs.h"
#include "shl_log.h"
#include "io.h"
#include "deserializer.h"
#include "vm.h"

int main(i32 argc, char **argv) {
  if (argc < 2) {
    ERROR("Bytecode file was not provided\n");
    return 1;
  }

  Str bytecode = read_file(argv[1]);
  if (bytecode.len == (u32) -1) {
    ERROR("File %s was not found\n", argv[1]);
    exit(1);
  }

  RcArena rc_arena = {0};
  Ir ir = deserialize((u8 *) bytecode.ptr, bytecode.len, &rc_arena);
  Intrinsics intrinsics = {0};
  execute(&ir, argc, argv, &rc_arena, &intrinsics);

  return 0;
}
