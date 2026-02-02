#include <locale.h>
#include <signal.h>

#ifdef CRYPTO
#include "sodium.h"
#endif
#include "aether/aether.h"
#include "aether/deserializer.h"
#include "aether/macros.h"

#ifndef NOSYSTEM
// From base.c
extern StringBuilder printf_sb;
#endif // NOSYSTEM
// From common.c
extern Arena intern_arena;

static Da(Vm) vms = {0};

void sigint_handler(i32 signal) {
  (void) signal;

  for (u32 i = 0; i < vms.len; ++i) {
#ifndef NOSYSTEM
    if (!vms.items[i].catch_kill_signal)
#endif
      vm_stop(vms.items + i);
  }
}

AetherCtx aether_init(i32 argc, char **argv, bool debug,
                      Intrinsics *intrinsics) {
  setlocale(LC_ALL, "");

  signal(SIGINT, sigint_handler);

#ifdef CRYPTO
  if (sodium_init() < 0)
    ERROR("Failed to initialize libsodium\n");
#endif

  AetherCtx ctx = {0};

  Intrinsics _intrinsics = {0};
  if (!intrinsics)
    intrinsics = &_intrinsics;

  Vm vm = vm_create(argc, argv, intrinsics);
  vm.max_trace_level = debug ? (u16) -1 : 0;

  ctx.vm_index = vms.len;
  DA_APPEND(vms, vm);

  return ctx;
}

Value *aether_eval(AetherCtx *ctx, Str code, Str file_path) {
  Str file_dir = get_file_dir(file_path);

  DA_APPEND(ctx->include_paths, file_dir);
  DA_APPEND(ctx->include_paths, STR_LIT("ae-src/"));
  DA_APPEND(ctx->include_paths, STR_LIT("/usr/include/aether/"));


  Vm *vm = vms.items + ctx->vm_index;

  vm->current_file_path = file_path;

  Exprs ast = parse_ex(code, &vm->current_file_path,
                       &ctx->macros, &ctx->included_files,
                       &ctx->include_paths, &ctx->asts,
                       &ctx->arena, false, NULL);

  expand_macros_block(&ast, &ctx->macros, NULL, NULL, false,
                      &ctx->arena, &vm->current_file_path,
                      0, 0, false);

  Ir ir = ast_to_ir(&ast, &ctx->arena);

  DA_APPEND(ctx->irs, ir);

  Value *result = execute_get(vm, ir);

  return result;
}

Value *aether_eval_bytecode(AetherCtx *ctx,
                            u8 *buffer, u32 size) {
  Vm *vm = vms.items + ctx->vm_index;

  Ir ir = deserialize(buffer, size, &ctx->arena,
                      &vm->current_file_path);

  DA_APPEND(ctx->irs, ir);

  Value *result = execute_get(vm, ir);

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

  for (u32 i = 0; i < ctx->irs.len; ++i)
    ir_free(ctx->irs.items[i]);
  free(ctx->irs.items);

  arena_free(&ctx->arena);

  free(ctx->asts.items);

  if (ctx->macros.items)
    free(ctx->macros.items);

  if (ctx->included_files.items)
    free(ctx->included_files.items);

  if (ctx->include_paths.items)
    free(ctx->include_paths.items);

  Vm *vm = vms.items + ctx->vm_index;

  vm_destroy(vm);

  if (vms.items)
    free(vms.items);
  vms.items = NULL;
  vms.len = 0;
  vms.cap = 0;
}

i32 aether_exit_code(AetherCtx *ctx) {
  return vms.items[ctx->vm_index].exit_code;
}
