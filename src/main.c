#include "io.h"
#include "aether/parser.h"
#include "aether/vm.h"
#include "aether/serializer.h"
#include "aether/deserializer.h"
#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl/shl-arena.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef struct {
  u32   flags_end;
  bool  compile_only;
  char *compiled_path;
  bool  load_from_file;
} Config;

Config parse_args(i32 argc, char **argv) {
  Config config = {0};

  for (u32 i = 1; i < (u32) argc; ++i) {
    config.flags_end = i;
    if (argv[i][0] != '-')
      break;

    if (strcmp(argv[i], "-c") == 0) {
      config.compile_only = true;
      if (i + 1 < (u32) argc) {
        config.compiled_path = argv[++i];
      } else {
        ERROR("Flag %s requires an argument\n", argv[i]);
        exit(1);
      }
    } else if (strcmp(argv[i], "-l") == 0) {
      config.load_from_file = true;
    } else {
      ERROR("Unknown flag %s\n", argv[i]);
      exit(1);
    }
  }

  return config;
}

int main(i32 argc, char **argv) {
  if (argc < 2) {
    ERROR("Input file was not provided\n");
    exit(1);
  }

  Config config = parse_args(argc, argv);

  char *input_file_path = argv[config.flags_end];
  Str code = read_file(input_file_path);
  if (code.len == (u32) -1) {
    ERROR("Could not open input file %s\n", input_file_path);
    exit(1);
  }

  RcArena rc_arena = {0};
  Ir ir;

  if (config.load_from_file)
    ir = deserialize((u8 *) code.ptr, code.len, &rc_arena);
  else
    ir = parse(code, input_file_path);

  if (config.compile_only) {
    Str bytecode = {0};
    bytecode.ptr = (char *) serialize(&ir, &bytecode.len);
    write_file(config.compiled_path, bytecode);

    return 0;
  }

  Intrinsics intrinsics = {0};

  return execute(&ir, argc, argv, &rc_arena, &intrinsics, NULL);
}
