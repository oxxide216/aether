#include "vm.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#include "shl_log.h"
#include "intrinsics.h"

static Value execute_block(Vm *vm, IrBlock *block);

static Intrinsic intrinsics[] = {
  { STR_LIT("print"), (u32) -1, &print_intrinsic },
};

static Value execute_func(Vm *vm, IrExprFuncCall *func) {
  Func *func_def = NULL;

  for (u32 i = 0; i < vm->funcs.len; ++i) {
    if (str_eq(vm->funcs.items[i].name, func->name) &&
        vm->funcs.items[i].args.len == func->args.len) {
      func_def = vm->funcs.items + i;
      break;
    }
  }

  for (u32 i = 0; i < ARRAY_LEN(intrinsics); ++i) {
    if (str_eq(intrinsics[i].name, func->name) &&
        (intrinsics[i].args_count == func->args.len ||
         intrinsics[i].args_count == (u32) -1)) {
      return intrinsics[i].func(vm, &func->args);
    }
  }

  if (!func_def) {
    ERROR("Function "STR_FMT" was not defined before usage\n",
          STR_ARG(func->name));
    exit(1);
  }

  u32 prev_vars_len = vm->vars.len;

  for (u32 i = 0; i < func_def->args.len; ++i) {
    Var var = {
      func_def->args.items[i],
      execute_expr(vm, func->args.items[i]),
    };
    DA_APPEND(vm->vars, var);
  }

  Value ret_val = execute_block(vm, &func_def->body);

  vm->vars.len = prev_vars_len;

  return ret_val;
}

Value execute_expr(Vm *vm, IrExpr *expr) {
  switch (expr->kind) {
  case IrExprKindFuncDef: {
    for (u32 i = 0; i < vm->funcs.len; ++i) {
      if (str_eq(vm->funcs.items[i].name, expr->as.func_def.name) &&
          vm->funcs.items[i].args.len == expr->as.func_def.args.len) {
        ERROR("Function "STR_FMT" was redefined\n",
              STR_ARG(expr->as.func_def.name));
        exit(1);
      }
    }

    DA_APPEND(vm->funcs, expr->as.func_def);
  } break;

  case IrExprKindFuncCall: {
    return execute_func(vm, &expr->as.func_call);
  } break;

  case IrExprKindList: {
    List list = {0};

    for (u32 i = 0; i < expr->as.list.content.len; ++i) {
      Value value = execute_expr(vm, expr->as.list.content.items[i]);
      DA_APPEND(list, value);
    }

    return (Value) {
      ValueKindList,
      { .list = list },
    };
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < vm->vars.len; ++i) {
      if (str_eq(vm->vars.items[i].name, expr->as.ident.ident))
        return vm->vars.items[i].value;
    }

    ERROR("Variable "STR_FMT" was not defined before usage\n",
          STR_ARG(expr->as.ident.ident));
    exit(1);
  } break;

  case IrExprKindStrLit: {
    return (Value) {
      ValueKindStrLit,
      { .str_lit = expr->as.str_lit.lit },
    };
  } break;

  case IrExprKindNumber: {
    return (Value) {
      ValueKindNumber,
      { .number = expr->as.number.number },
    };
  } break;

  case IrExprKindVarDef: {
    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value = execute_expr(vm, expr->as.var_def.expr);
    DA_APPEND(vm->vars, var);

    return var.value;
  } break;
  }

  return (Value) { ValueKindUnit, {} };
}

static Value execute_block(Vm *vm, IrBlock *block) {
  for (u32 i = 0; i + 1 < block->len; ++i)
    execute_expr(vm, block->items[i]);

  if (block->len > 0)
    return execute_expr(vm, block->items[block->len - 1]);
  return (Value) { ValueKindUnit, {} };
}

void execute(Ir *ir) {
  Vm vm = {0};
  execute_block(&vm, ir);
}
