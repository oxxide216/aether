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
  Ir ir = deserialize((u8 *) bytecode.ptr, bytecode.len);
  execute(&ir);

  return 0;
}
