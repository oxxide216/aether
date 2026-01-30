#include <locale.h>

#include "aether/aether.h"
#include "aether/deserializer.h"
#include "aether/macros.h"

#ifndef NOSYSTEM
// From base.c
extern StringBuilder printf_sb;
#endif // NOSYSTEM
// From common.c
extern Arena intern_arena;

AetherCtx aether_init(i32 argc, char **argv, bool debug,
                      Intrinsics *intrinsics) {
  setlocale(LC_ALL, "");

  AetherCtx ctx = {0};

  Intrinsics _intrinsics = {0};
  if (!intrinsics)
    intrinsics = &_intrinsics;

  ctx.vm = vm_create(argc, argv, intrinsics);
  ctx.vm.max_trace_level = debug ? (u16) -1 : 0;

  return ctx;
}

Value *aether_eval(AetherCtx *ctx, Str code,
                   Str file_path, bool value_expected) {
  Str file_dir = get_file_dir(file_path);

  DA_APPEND(ctx->include_paths, file_dir);
  DA_APPEND(ctx->include_paths, STR_LIT("ae-src/"));
  DA_APPEND(ctx->include_paths, STR_LIT("/usr/include/aether/"));

  ctx->vm.current_file_path = file_path;

  Exprs ast = parse_ex(code, &ctx->vm.current_file_path,
                       &ctx->macros, &ctx->included_files,
                       &ctx->include_paths, &ctx->asts,
                       &ctx->arena, false, NULL);

  expand_macros_block(&ast, &ctx->macros, NULL, NULL, false,
                      &ctx->arena, &ctx->vm.current_file_path,
                      0, 0, false);

  Ir ir = ast_to_ir(&ast, &ctx->arena);

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

void aether_eval_macros(AetherCtx *ctx,
                        u8 *macros_buffer,
                        u32 macros_len) {
  Macros macros = deserialize_macros(macros_buffer,
                                     macros_len,
                                     NULL, &ctx->arena);

  if (ctx->macros.cap < ctx->macros.len + macros.len) {
    ctx->macros.cap = ctx->macros.len + macros.len;

    if (ctx->macros.len == 0)
      ctx->macros.items = malloc(ctx->macros.cap * sizeof(Macro));
    else
      ctx->macros.items = realloc(ctx->macros.items,
                                  ctx->macros.cap * sizeof(Macro));
  }

  memcpy(ctx->macros.items + ctx->macros.len,
         macros.items, macros.len * sizeof(Macro));
  ctx->macros.len += macros.len;
}

void aether_cleanup(AetherCtx *ctx) {
#ifndef NOSYSTEM
  if (printf_sb.buffer)
    free(printf_sb.buffer);
  printf_sb.buffer = NULL;
  printf_sb.len = 0;
  printf_sb.cap = 0;
#endif

  arena_free(&intern_arena);

  arena_free(&ctx->arena);

  free(ctx->asts.items);

  if (ctx->macros.items)
    free(ctx->macros.items);

  if (ctx->included_files.items)
    free(ctx->included_files.items);

  if (ctx->include_paths.items)
    free(ctx->include_paths.items);

  vm_destroy(&ctx->vm);
}
