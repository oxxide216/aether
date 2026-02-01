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
  Vm           vm;
} AetherCtx;

AetherCtx  aether_init(i32 argc, char **argv, bool debug,
                       Intrinsics *intrinsics);
Value     *aether_eval(AetherCtx *ctx, Str code, Str file_path);
Value     *aether_eval_bytecode(AetherCtx *ctx,
                                u8 *buffer, u32 size);
void       aether_eval_macros(AetherCtx *ctx,
                              u8 *macros_buffer,
                              u32 macros_len);
void       aether_cleanup(AetherCtx *ctx);

#endif // AETHER_H
