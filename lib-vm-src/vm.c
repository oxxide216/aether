#include "vm.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#include "shl_log.h"
#include "shl_arena.h"
#include "intrinsics.h"

static Value execute_block(Vm *vm, IrBlock *block);

static Value execute_func(Vm *vm, IrExprFuncCall *func) {
  Func *func_def = NULL;

  for (u32 i = 0; i < vm->funcs.len; ++i) {
    if (str_eq(vm->funcs.items[i].name, func->name) &&
        vm->funcs.items[i].args.len == func->args.len) {
      func_def = vm->funcs.items + i;
      break;
    }
  }

  for (u32 i = 0; i < vm->intrinsics.len; ++i) {
    if (str_eq(vm->intrinsics.items[i].name, func->name) &&
        (vm->intrinsics.items[i].args_count == func->args.len ||
         vm->intrinsics.items[i].args_count == (u32) -1)) {
      return vm->intrinsics.items[i].func(vm, &func->args);
    }
  }

  if (!func_def) {
    ERROR("Function "STR_FMT" with %u arguments was not defined before usage\n",
          STR_ARG(func->name), func->args.len);
    exit(1);
  }

  Vars new_local_vars = {0};
  for (u32 i = 0; i < func_def->args.len; ++i) {
    Var var = {
      func_def->args.items[i],
      execute_expr(vm, func->args.items[i]),
    };
    DA_APPEND(new_local_vars, var);
  }

  Vars prev_local_vars = vm->local_vars;
  bool prev_is_inside_of_func = vm->is_inside_of_func;
  vm->local_vars = new_local_vars;
  vm->is_inside_of_func = true;

  Value ret_val = execute_block(vm, &func_def->body);

  free(vm->local_vars.items);
  vm->local_vars = prev_local_vars;
  vm->is_inside_of_func = prev_is_inside_of_func;

  return ret_val;
}

static Var *get_var(Vm *vm, Str name) {
  for (u32 i = vm->local_vars.len; i > 0; --i) {
    if (str_eq(vm->local_vars.items[i - 1].name, name))
      return vm->local_vars.items + i - 1;
  }

  for (u32 i = vm->global_vars.len; i > 0; --i) {
    if (str_eq(vm->global_vars.items[i - 1].name, name))
      return vm->global_vars.items + i - 1;
  }

  return NULL;
}

static bool value_eq(Value *a, Value *b) {
  if (a->kind != b->kind)
    return false;

  if (a->kind == ValueKindList &&
      a->as.list != b->as.list)
    return false;

  if (a->kind == ValueKindStr &&
      !str_eq(a->as.str, b->as.str))
    return false;

  if (a->kind == ValueKindNumber &&
      a->as.number != b->as.number)
    return false;

  if (a->kind == ValueKindBool &&
      a->as._bool != b->as._bool)
    return false;

  return true;
}

static void free_value(Value *value, RcArena *rc_arena) {
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list;
    while (node) {
      ListNode *next_node = node->next;
      rc_arena_free(rc_arena, node);
      node = next_node;
    }
  } else if (value->kind == ValueKindStr) {
    rc_arena_free(rc_arena, value->as.str.ptr);
  }
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

  case IrExprKindVarDef: {
    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value = execute_expr(vm, expr->as.var_def.expr);

    Var *prev_var = get_var(vm, var.name);
    if (prev_var && !value_eq(&prev_var->value, &var.value)) {
      free_value(&prev_var->value, vm->rc_arena);
    }

    if (vm->is_inside_of_func)
      DA_APPEND(vm->local_vars, var);
    else
      DA_APPEND(vm->global_vars, var);

    return var.value;
  } break;

  case IrExprKindIf: {
    Value cond = execute_expr(vm, expr->as._if.cond);
    if (cond.kind != ValueKindBool) {
      ERROR("Only boolean value can be used as a condition\n");
      exit(1);
    }

    if (cond.as._bool)
      return execute_block(vm, &expr->as._if.if_body);

    if (expr->as._if.has_else)
      return execute_block(vm, &expr->as._if.else_body);

    return (Value) { ValueKindUnit, {} };
  } break;

  case IrExprKindList: {
    ListNode *list = NULL;
    ListNode *list_end = NULL;

    for (u32 i = 0; i < expr->as.list.content.len; ++i) {
      ListNode *new_node = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      new_node->value = execute_expr(vm, expr->as.list.content.items[i]);
      new_node->next = NULL;

      if (list_end) {
        list_end->next = new_node;
        list_end = new_node;
      } else {
        list = new_node;
        list_end = new_node;
      }
    }

    return (Value) {
      ValueKindList,
      { .list = list },
    };
  } break;

  case IrExprKindIdent: {
    for (u32 i = vm->local_vars.len; i > 0; --i) {
      if (str_eq(vm->local_vars.items[i - 1].name, expr->as.ident.ident))
        return vm->local_vars.items[i - 1].value;
    }

    for (u32 i = vm->global_vars.len; i > 0; --i) {
      if (str_eq(vm->global_vars.items[i - 1].name, expr->as.ident.ident))
        return vm->global_vars.items[i - 1].value;
    }

    Var *var = get_var(vm, expr->as.ident.ident);
    if (!var) {
      ERROR("Variable "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.ident.ident));
      exit(1);
    }
  } break;

  case IrExprKindStrLit: {
    return (Value) {
      ValueKindStr,
      { .str = expr->as.str_lit.lit },
    };
  } break;

  case IrExprKindNumber: {
    return (Value) {
      ValueKindNumber,
      { .number = expr->as.number.number },
    };
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

void execute(Ir *ir, i32 argc, char **argv,
             RcArena *rc_arena, Intrinsics *intrinsics) {
  Vm vm = {0};
  vm.rc_arena = rc_arena;
  vm.intrinsics = *intrinsics;

  ListNode *args_end = NULL;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = rc_arena_alloc(rc_arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = aalloc(sizeof(ListNode));
    new_arg->value = (Value) {
      ValueKindStr,
      { .str = { buffer, len } },
    };

    if (args_end) {
      args_end->next = new_arg;
      args_end = new_arg;
    } else {
      vm.args = new_arg;
      args_end = new_arg;
    }
  }

  execute_block(&vm, ir);
}
