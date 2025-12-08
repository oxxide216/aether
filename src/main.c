#include <signal.h>

#include "io.h"
#include "aether/parser.h"
#include "aether/deserializer.h"
#include "aether/common.h"
#include "aether/vm.h"
#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef struct {
  char *cstr;
  bool  is_precompiled;
} Path;

extern InternStrings intern_strings;
extern CachedIrs cached_irs;
#ifndef NOSYSTEM
extern StringBuilder printf_sb;
#endif

static Path loader_paths[] = {
  { "/usr/include/aether/load/loader.abc", true },
  { "ae-src/loader.ae", false },
};

static Arena arena = {0};
static Vm vm = {0};

void cleanup(void) {
  if (intern_strings.items)
    free(intern_strings.items);

  for (u32 i = 0; i < cached_irs.len; ++i)
    arena_free(&cached_irs.items[i].arena);
  free(cached_irs.items);

  free(printf_sb.buffer);

  arena_free(&arena);
  vm_destroy(&vm);
}

void sigint_handler(i32 signal) {
  (void) signal;

  vm_stop(&vm);
}

i32 main(i32 argc, char **argv) {
  Str code = { NULL, (u32) -1 };
  Path *path = NULL;

  for (u32 i = 0; i < ARRAY_LEN(loader_paths); ++i) {
    code = read_file(loader_paths[i].cstr);

    if (code.len != (u32) -1) {
      path = loader_paths + i;
      break;
    }
  }

  if (code.len == (u32) -1) {
    ERROR("Loader file was not found\n");
    exit(1);
  }

  signal(SIGINT, sigint_handler);

  Ir ir;
  if (path->is_precompiled)
    ir = deserialize((u8 *) code.ptr, code.len, &arena);
  else
    ir = parse(code, path->cstr);

  free(code.ptr);

  Intrinsics intrinsics = {0};
  vm = vm_create(argc, argv, &intrinsics);
  vm.current_file_path = STR(path->cstr, strlen(path->cstr));
  execute_block(&vm, &ir, false);

  cleanup();

  return vm.exit_code;
}
