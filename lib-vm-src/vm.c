#include "vm.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#include "shl_log.h"
#include "shl_arena.h"
#include "intrinsics.h"

void value_stack_push_unit(ValueStack *stack) {
  Value value = { ValueKindUnit, {} };
  DA_APPEND(*stack, value);
}

void value_stack_push_list(ValueStack *stack, ListNode *nodes) {
  Value value = { ValueKindList, { .list = nodes } };
  DA_APPEND(*stack, value);
}

void value_stack_push_str(ValueStack *stack, Str str) {
  Value value = { ValueKindStr, { .str = str } };
  DA_APPEND(*stack, value);
}

void value_stack_push_number(ValueStack *stack, i64 number) {
  Value value = { ValueKindNumber, { .number = number } };
  DA_APPEND(*stack, value);
}

void value_stack_push_bool(ValueStack *stack, bool _bool) {
  Value value = { ValueKindBool, { ._bool = _bool } };
  DA_APPEND(*stack, value);
}

Value value_stack_pop(ValueStack *stack) {
  if (stack->len > 0)
    return stack->items[--stack->len];
  return (Value) { ValueKindUnit, {} };
}

Value *value_stack_get(ValueStack *stack, u32 index) {
  return stack->items + index;
}

static void execute_block(Vm *vm, IrBlock *block, bool value_expected);

static Func *get_func(Vm *vm, Str name, u32 args_count) {
  for (u32 i = vm->local_vars.len; i > 0; --i) {
    Var *var = vm->local_vars.items + i - 1;
    Value *value = vm->stack.items + var->value_index;

    if (str_eq(var->name, name) &&
        value->kind == ValueKindFunc) {
      if (value->as.func->args.len == args_count)
        return value->as.func;
      else
        break;
    }
  }

  for (u32 i = vm->global_vars.len; i > 0; --i) {
    Var *var = vm->global_vars.items + i - 1;
    Value *value = vm->stack.items + var->value_index;

    if (str_eq(var->name, name) &&
        value->kind == ValueKindFunc) {
      if (value->as.func->args.len == args_count)
        return value->as.func;
      else
        break;
    }
  }

  for (u32 i = 0; i < vm->funcs.len; ++i) {
    Func *func = vm->funcs.items + i;

    if (str_eq(func->name, name) &&
        func->args.len == args_count)
      return func;
  }

  return NULL;
}

static void execute_func(Vm *vm, IrExprFuncCall *func, bool value_expected) {
  Func *func_def = get_func(vm, func->name, func->args.len);
  if (!func_def) {
    for (u32 i = 0; i < vm->intrinsics.len; ++i) {
      Intrinsic *intrinsic = vm->intrinsics.items + i;

      if (str_eq(intrinsic->name, func->name) &&
          (intrinsic->args_count == func->args.len ||
           intrinsic->args_count == (u32) -1)) {
        u32 prev_stack_len = vm->stack.len;

        for (u32 i = 0; i < func->args.len; ++i)
          execute_expr(vm, func->args.items[i], true);

        intrinsic->func(vm);

        if (value_expected && !intrinsic->has_return_value)
          value_stack_push_unit(&vm->stack);

        vm->stack.len = prev_stack_len + value_expected;

        return;
      }
    }

    ERROR("Function "STR_FMT" with %u arguments was not defined before usage\n",
          STR_ARG(func->name), func->args.len);
    exit(1);
  }

  u32 prev_stack_len = vm->stack.len;

  Vars new_local_vars = {0};
  for (u32 i = 0; i < func_def->args.len; ++i) {
    Var var = {
      func_def->args.items[i],
      vm->stack.len,
    };

    execute_expr(vm, func->args.items[i], true);

    DA_APPEND(new_local_vars, var);
  }

  Vars prev_local_vars = vm->local_vars;
  bool prev_is_inside_of_func = vm->is_inside_of_func;
  vm->local_vars = new_local_vars;
  vm->is_inside_of_func = true;

  execute_block(vm, &func_def->body, value_expected);

  free(vm->local_vars.items);
  vm->local_vars = prev_local_vars;
  vm->is_inside_of_func = prev_is_inside_of_func;

  vm->stack.len = prev_stack_len;
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

static void free_value(Value *value, RcArena *rc_arena) {
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list;
    while (node && !node->is_static) {
      ListNode *next_node = node->next;
      rc_arena_free(rc_arena, node);
      node = next_node;
    }
  } else if (value->kind == ValueKindStr) {
    rc_arena_free(rc_arena, value->as.str.ptr);
  }
}

void execute_expr(Vm *vm, IrExpr *expr, bool value_expected) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    execute_block(vm, &expr->as.block, value_expected);
  } break;

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

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindFuncCall: {
    execute_func(vm, &expr->as.func_call, value_expected);
  } break;

  case IrExprKindVarDef: {
    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value_index = vm->stack.len;

    execute_expr(vm, expr->as.var_def.expr, true);

    Var *prev_var = get_var(vm, var.name);
    if (prev_var) {
      Value *prev_value = vm->stack.items + prev_var->value_index;
      free_value(prev_value, vm->rc_arena);
      *prev_value = vm->stack.items[--vm->stack.len];
    }

    if (vm->is_inside_of_func)
      DA_APPEND(vm->local_vars, var);
    else
      DA_APPEND(vm->global_vars, var);
  } break;

  case IrExprKindIf: {
    execute_expr(vm, expr->as._if.cond, true);

    Value cond = value_stack_pop(&vm->stack);
    if (cond.kind != ValueKindBool) {
      ERROR("Only boolean value can be used as a condition\n");
      exit(1);
    }

    if (cond.as._bool)
      execute_block(vm, &expr->as._if.if_body, value_expected);
    else if (expr->as._if.has_else)
      execute_block(vm, &expr->as._if.else_body, value_expected);
  } break;

  case IrExprKindList: {
    if (!value_expected)
      break;

    ListNode *list = NULL;
    ListNode *list_end = NULL;

    for (u32 i = 0; i < expr->as.list.content.len; ++i) {
      execute_expr(vm, expr->as.list.content.items[i], true);

      ListNode *new_node = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      new_node->value = value_stack_pop(&vm->stack);
      new_node->next = NULL;

      if (list_end) {
        list_end->next = new_node;
        list_end = new_node;
      } else {
        list = new_node;
        list_end = new_node;
      }
    }

    value_stack_push_list(&vm->stack, list);
  } break;

  case IrExprKindIdent: {
    if (!value_expected)
      break;

    for (u32 i = vm->local_vars.len; i > 0; --i) {
      if (str_eq(vm->local_vars.items[i - 1].name, expr->as.ident.ident)) {
        u32 index = vm->local_vars.items[i - 1].value_index;
        DA_APPEND(vm->stack, vm->stack.items[index]);
        return;
      }
    }

    for (u32 i = vm->global_vars.len; i > 0; --i) {
      if (str_eq(vm->global_vars.items[i - 1].name, expr->as.ident.ident)) {
        u32 index = vm->global_vars.items[i - 1].value_index;
        DA_APPEND(vm->stack, vm->stack.items[index]);
      }
    }

    for (u32 i = 0; i < vm->funcs.len; ++i) {
      Func *func = vm->funcs.items + i;

      if (str_eq(func->name, expr->as.ident.ident)) {
        Value func_value = { ValueKindFunc, { .func = func } };
        DA_APPEND(vm->stack, func_value);
        return;
      }
    }

    Var *var = get_var(vm, expr->as.ident.ident);
    if (!var) {
      ERROR("Variable "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.ident.ident));
      exit(1);
    }
  } break;

  case IrExprKindStrLit: {
    if (value_expected)
      value_stack_push_str(&vm->stack, expr->as.str_lit.lit);
  } break;

  case IrExprKindNumber: {
    if (value_expected)
      value_stack_push_number(&vm->stack, expr->as.number.number);
  } break;

  case IrExprKindBool: {
    if (value_expected)
      value_stack_push_bool(&vm->stack, expr->as._bool._bool);
  } break;
  }
}

static void execute_block(Vm *vm, IrBlock *block, bool value_expected) {
  for (u32 i = 0; i + 1 < block->len; ++i)
    execute_expr(vm, block->items[i], false);

  if (block->len > 0)
    execute_expr(vm, block->items[block->len - 1], value_expected);
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

    ListNode *new_arg = rc_arena_alloc(rc_arena, sizeof(ListNode));
    new_arg->value = (Value) {
      ValueKindStr,
      { .str = { buffer, len } },
    };
    new_arg->is_static = true;

    if (args_end) {
      args_end->next = new_arg;
      args_end = new_arg;
    } else {
      vm.args = new_arg;
      args_end = new_arg;
    }
  }

  execute_block(&vm, ir, false);
}
