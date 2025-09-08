#include "io.h"
#include "parser.h"
#include "serializer.h"
#include "shl_defs.h"
#include "shl_log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"

int main(i32 argc, char **argv) {
  if (argc < 2) {
    ERROR("Output file was not provided\n");
    return 1;
  }

  if (argc < 3) {
    ERROR("Input file was not provided\n");
    return 1;
  }

  char *output_file_name = argv[1];
  char *input_file_name = argv[2];

  Str code = read_file(input_file_name);
  if (code.len == (u32) -1) {
    ERROR("File %s was not found\n", input_file_name);
    exit(1);
  }

  Ir ir = parse(code);
  Str serialized = {0};
  serialized.ptr = (char *) serialize(&ir, &serialized.len);
  write_file(output_file_name, serialized);

  return 0;
}
