#include "aether/aether.h"
#include "aether/io.h"
#include "shl/shl-defs.h"
#include "shl/shl-log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef struct {
  char *cstr;
  bool  is_precompiled;
} Path;

static Path loader_paths[] = {
  { "ae-src/loader.ae", false },
  { "/usr/local/include/aether/loader.abc", true },
};

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

#ifdef NDEBUG
  bool debug = false;
#else
  bool debug = true;
#endif

  AetherCtx ctx = aether_init(argc, argv, debug, NULL);

  if (path->is_precompiled) {
    aether_eval_bytecode(&ctx, (u8 *) code.ptr, code.len);
  } else {
    Str file_path = { path->cstr, strlen(path->cstr) };
    aether_eval(&ctx, code, file_path);
  }

  free(code.ptr);

  aether_cleanup(&ctx);

  return ctx.vm.exit_code;
}
