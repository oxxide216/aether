#include <locale.h>

#include "aether/aether.h"
#include "aether/deserializer.h"

#ifndef NOSYSTEM
// From base.c
extern StringBuilder printf_sb;
#endif // NOSYSTEM

AetherCtx aether_init(i32 argc, char **argv, bool debug,
                      Intrinsics *intrinsics) {
  setlocale(LC_ALL, "");

  AetherCtx ctx = {0};

  Intrinsics _intrinsics = {0};
  if (!intrinsics)
    intrinsics = &_intrinsics;

  ctx.vm = vm_create(argc, argv, intrinsics);
  ctx.vm.tracing_disabled = !debug;

  return ctx;
}

Value *aether_eval(AetherCtx *ctx, Str code,
                   Str file_path, bool value_expected) {
  ctx->vm.current_file_path = file_path;
  Ir ir = parse(code, &ctx->vm.current_file_path, &ctx->asts);

  Value *result = execute(&ctx->vm, &ir, value_expected);

  for (u32 i = 0; i < ir.len; ++i)
    free(ir.items[i].instrs.items);
  free(ir.items);

  return result;
}

Value *aether_eval_bytecode(AetherCtx *ctx,
                            u8 *buffer, u32 size,
                            bool value_expected) {
  Ir ir = deserialize(buffer, size, &ctx->arena,
                      &ctx->vm.current_file_path);

  Value *result = execute(&ctx->vm, &ir, value_expected);

  for (u32 i = 0; i < ir.len; ++i)
    free(ir.items[i].instrs.items);
  free(ir.items);

  return result;
}

void aether_cleanup(AetherCtx *ctx) {
#ifndef NOSYSTEM
  if (printf_sb.buffer)
    free(printf_sb.buffer);
#endif

  arena_free(&ctx->arena);
  vm_destroy(&ctx->vm);

  for (u32 i = 0; i < ctx->asts.len; ++i)
    arena_free(&ctx->asts.items[i].arena);

  free(ctx->asts.items);
}
