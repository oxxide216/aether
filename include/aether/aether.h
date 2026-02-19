#ifndef AETHER_H
#define AETHER_H

#include "aether/vm.h"
#include "shl/shl-defs.h"

typedef struct {
  Arena        arena;
  CachedASTs   asts;
  CachedIrs    irs;
  Macros       macros;
  FilePaths    included_files;
  IncludePaths include_paths;
  u32          vm_index;
} AetherCtx;

AetherCtx  aether_init(i32 argc, char **argv, Intrinsics *intrinsics);
Value     *aether_eval(AetherCtx *ctx, Str code, Str file_path);
Value     *aether_eval_bytecode(AetherCtx *ctx, u8 *buffer, u32 size);
void       aether_eval_macros(AetherCtx *ctx, u8 *macros_buffer, u32 macros_len);
void       aether_cleanup(AetherCtx *ctx);
i32        aether_exit_code(AetherCtx *ctx);

#endif // AETHER_H
