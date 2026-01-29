#ifndef AETHER_H
#define AETHER_H

#include "aether/vm.h"
#include "shl/shl-defs.h"

typedef struct {
  Arena      arena;
  CachedASTs asts;
  Vm         vm;
} AetherCtx;

AetherCtx  aether_init(i32 argc, char **argv, bool debug,
                       Intrinsics *intrinsics);
Value     *aether_eval(AetherCtx *ctx, Str code,
                       Str file_path, bool value_expected);
Value     *aether_eval_bytecode(AetherCtx *ctx,
                                u8 *buffer, u32 size,
                                bool value_expected);
void       aether_cleanup(AetherCtx *ctx);

#endif // AETHER_H
