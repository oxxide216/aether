#include <signal.h>
#include <locale.h>

#include "aether/parser.h"
#include "aether/deserializer.h"
#include "aether/common.h"
#include "aether/vm.h"
#include "aether/io.h"
#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef struct {
  char *cstr;
  bool  is_precompiled;
  bool  tracing_enabled;
} Path;

// From common.c
extern InternStrings intern_strings;
#ifndef NOSYSTEM
// From base.c
extern StringBuilder printf_sb;
// From term.c
extern bool catch_kill;
#endif

static Path loader_paths[] = {
  { "ae-src/loader.ae", false, true },
  { "/usr/local/include/aether/loader.abc", true, false },
};

static CachedASTs cached_asts = {0};
static Arena arena = {0};
static Ir ir = {0};
static Vm vm = {0};

void cleanup(void) {
  if (intern_strings.items)
    free(intern_strings.items);

  if (printf_sb.buffer)
    free(printf_sb.buffer);

  arena_free(&arena);
  vm_destroy(&vm);

  for (u32 i = 0; i < cached_asts.len; ++i)
    arena_free(&cached_asts.items[i].arena);

  free(cached_asts.items);
}

void sigint_handler(i32 signal) {
  (void) signal;

#ifndef NOSYSTEM
  if (!catch_kill)
#endif
    vm_stop(&vm);
}

i32 main(i32 argc, char **argv) {
  setlocale(LC_ALL, "");

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

  Intrinsics intrinsics = {0};
  vm = vm_create(argc, argv, &intrinsics);
  vm.tracing_enabled = path->tracing_enabled;

  if (path->is_precompiled) {
    ir = deserialize((u8 *) code.ptr, code.len, &arena, &vm.current_file_path);
  } else {
    vm.current_file_path = (Str) { path->cstr, strlen(path->cstr) };
    ir = parse(code, &vm.current_file_path, &cached_asts);
  }

  signal(SIGINT, sigint_handler);

  execute(&vm, &ir, false);

  free(code.ptr);
  cleanup();

  return vm.exit_code;
}
