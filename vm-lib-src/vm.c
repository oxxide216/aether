#include "aether/vm.h"
#include "shl_str.h"
#include "shl_log.h"
#include "shl_arena.h"
#include "base/intrinsics.h"
#ifndef NOSYSTEM
#include "system/intrinsics.h"
#endif
#ifndef NOIUI
#include "iui/intrinsics.h"
#endif

typedef Da(Str) Strs;

void list_use(RcArena *rc_arena, ListNode *list) {
  while (list) {
    rc_arena_clone(rc_arena, list);

    list = list->next;
  }
}

ListNode *list_clone(RcArena *rc_arena, ListNode *list) {
  if (!list)
    return NULL;

  ListNode *new_list = NULL;
  ListNode **new_list_next = &new_list;
  ListNode *node = list;
  while (node) {
    *new_list_next = rc_arena_alloc(rc_arena, sizeof(ListNode));
    (*new_list_next)->value = node->value;
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  return new_list;
}

void value_stack_push_unit(ValueStack *stack) {
  Value value = { ValueKindUnit, {} };
  DA_APPEND(*stack, value);
}

void value_stack_push_list(ValueStack *stack, ListNode *nodes) {
  Value value = { ValueKindList, { .list = nodes } };
  DA_APPEND(*stack, value);
}

void value_stack_push_string(ValueStack *stack, Str string) {
  Value value = {
    ValueKindString,
    { .string = { { string.ptr, string.len }, (Str *) string.ptr } },
  };
  DA_APPEND(*stack, value);
}

void value_stack_push_int(ValueStack *stack, i64 _int) {
  Value value = { ValueKindInt, { ._int = _int } };
  DA_APPEND(*stack, value);
}

void value_stack_push_float(ValueStack *stack, f64 _float) {
  Value value = { ValueKindFloat, { ._float = _float } };
  DA_APPEND(*stack, value);
}

void value_stack_push_bool(ValueStack *stack, bool _bool) {
  Value value = { ValueKindBool, { ._bool = _bool } };
  DA_APPEND(*stack, value);
}

void value_stack_push_record(ValueStack *stack, Record record) {
  Value value = { ValueKindRecord, { .record = record } };
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

void free_value(Value *value, RcArena *rc_arena) {
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list;
    while (node && !node->is_static) {
      ListNode *next_node = node->next;
      rc_arena_free(rc_arena, node);
      node = next_node;
    }
  } else if (value->kind == ValueKindString) {
    rc_arena_free(rc_arena, value->as.string.begin);
  } else if (value->kind == ValueKindRecord) {
    for (u32 i = 0; i < value->as.record.len; ++i)
      free_value(&value->as.record.items[i].value, rc_arena);
  }
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

static ExecState catch_vars_block(Vm *vm, Strs *local_names,
                                  NamedValues *catched_values,
                                  IrBlock *block);

#define CATCH_VARS(vm, local_names, catched_values, expr)                \
  do {                                                                   \
    ExecState state = catch_vars(vm, local_names, catched_values, expr); \
    if (state != ExecStateContinue)                                      \
      return state;                                                      \
  } while (0)

#define CATCH_VARS_BLOCK(vm, local_names, catched_values, block)                \
  do {                                                                          \
    ExecState state = catch_vars_block(vm, local_names, catched_values, block); \
    if (state != ExecStateContinue)                                             \
      return state;                                                             \
  } while (0)

static ExecState catch_vars(Vm *vm, Strs *local_names,
                            NamedValues *catched_values,
                            IrExpr *expr) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as.block);
  } break;

  case IrExprKindFuncCall: {
    CATCH_VARS(vm, local_names, catched_values, expr->as.func_call.func);
    CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as.func_call.args);
  } break;

  case IrExprKindVarDef: {
    CATCH_VARS(vm, local_names, catched_values, expr->as.var_def.expr);

    DA_APPEND(*local_names, expr->as.var_def.name);
  } break;

  case IrExprKindIf: {
    CATCH_VARS(vm, local_names, catched_values, expr->as._if.cond);
    CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as._if.if_body);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i)
      CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as._if.elifs.items[i].body);

    if (expr->as._if.has_else)
      CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as._if.else_body);
  } break;

  case IrExprKindWhile: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as._while.body);
  } break;

  case IrExprKindSet: {
    CATCH_VARS(vm, local_names, catched_values, expr->as.set.src);
  } break;

  case IrExprKindField: {
    if (expr->as.field.is_set)
      CATCH_VARS(vm, local_names, catched_values, expr->as.field.expr);
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      CATCH_VARS(vm, local_names, catched_values, expr->as.ret.expr);
  } break;

  case IrExprKindList: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as.list.content);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.ident.ident, local_names->items[i]))
        return ExecStateContinue;

    Var *var = get_var(vm, expr->as.ident.ident);
    if (!var) {
      ERROR("Symbol "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.ident.ident));
      vm->exit_code = 1;
      return ExecStateExit;
    }

    if (!var->is_global) {
      NamedValue value = { var->name, var->value };
      DA_APPEND(*catched_values, value);
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      DA_APPEND(*local_names, expr->as.lambda.args.items[i]);

    CATCH_VARS_BLOCK(vm, local_names, catched_values, &expr->as.lambda.body);
  } break;

  case IrExprKindRecord: {
    for (u32 i = 0; i < expr->as.record.len; ++i)
      CATCH_VARS(vm, local_names, catched_values, expr->as.record.items[i].expr);
  } break;

  case IrExprKindSelfCall: {
    for (u32 i = 0; i < expr->as.self_call.args.len; ++i)
      CATCH_VARS(vm, local_names, catched_values, expr->as.self_call.args.items[i]);
  } break;
  }

  return ExecStateContinue;
}

static ExecState catch_vars_block(Vm *vm, Strs *local_names,
                                  NamedValues *catched_values,
                                  IrBlock *block) {
  for (u32 i = 0; i < block->len; ++i)
    CATCH_VARS(vm, local_names, catched_values, block->items[i]);

  return ExecStateContinue;
}

bool value_list_matches_kinds(u32 len, Value *values, ValueKind *kinds) {
  for (u32 i = 0; i < len; ++i)
    if (values[i].kind != kinds[i] && kinds[i] != ValueKindUnit)
      return false;

  return true;
}

static Intrinsic *get_intrinsic(Vm *vm, Str name, u32 args_count, Value *args) {
  for (u32 i = 0; i < vm->intrinsics.len; ++i) {
    if (str_eq(vm->intrinsics.items[i].name, name) &&
        vm->intrinsics.items[i].args_count == args_count &&
        value_list_matches_kinds(args_count, args,
                                 vm->intrinsics.items[i].arg_kinds)) {
      return vm->intrinsics.items + i;
    }
  }

  return NULL;
}

ExecState execute_func(Vm *vm, Func *func, bool value_expected) {
  if (vm->stack.len < func->args.len) {
    ERROR("Not enough values on the stack for function call\n");
    exit(1);
  }

  if (func->intrinsic_name.len > 0) {
    Intrinsic *intrinsic = get_intrinsic(vm, func->intrinsic_name, func->args.len,
                                         vm->stack.items + vm->stack.len - func->args.len);
    if (!intrinsic) {
      ERROR("Intrinsic `"STR_FMT"` with such signature was not found\n",
            STR_ARG(func->intrinsic_name));
      return ExecStateExit;
    }

    if (!(*intrinsic->func)(vm))
      return ExecStateExit;

    if (value_expected && !intrinsic->has_return_value)
      value_stack_push_unit(&vm->stack);
    else if (!value_expected && intrinsic->has_return_value)
      --vm->stack.len;

    return ExecStateContinue;
  }

  u32 prev_stack_len = vm->stack.len;
  Vars prev_local_vars = vm->local_vars;
  bool prev_is_inside_of_func = vm->is_inside_of_func;
  Func prev_current_func_value = vm->current_func_value;

  vm->local_vars = (Vars) {0};
  vm->is_inside_of_func = true;
  vm->current_func_value = *func;

  for (u32 i = 0; i < func->args.len; ++i) {
    Var var = {
      func->args.items[i],
      vm->stack.items[vm->stack.len - func->args.len + i],
      false,
    };

    DA_APPEND(vm->local_vars, var);
  }

  for (u32 i = 0; i < func->catched_values.len; ++i) {
    Var var = {
      func->catched_values.items[i].name,
      func->catched_values.items[i].value,
      false,
    };

    DA_APPEND(vm->local_vars, var);
  }

  ExecState result = execute_block(vm, &func->body, value_expected);

  for (u32 i = prev_stack_len; i < vm->stack.len - value_expected; ++i)
    free_value(vm->stack.items + i, vm->rc_arena);

  if (vm->local_vars.items)
    free(vm->local_vars.items);
  vm->local_vars = prev_local_vars;
  vm->is_inside_of_func = prev_is_inside_of_func;
  vm->current_func_value = prev_current_func_value;

  if (value_expected)
    vm->stack.items[prev_stack_len - func->args.len] = vm->stack.items[vm->stack.len - 1];
  vm->stack.len = prev_stack_len - func->args.len + value_expected;

  if (result == ExecStateExit)
    return result;
  return ExecStateContinue;
}

ExecState execute_expr(Vm *vm, IrExpr *expr, bool value_expected) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    EXECUTE_BLOCK(vm, &expr->as.block, value_expected);
  } break;

  case IrExprKindFuncCall: {
    EXECUTE_EXPR(vm, expr->as.func_call.func, true);

    Value func_value = value_stack_pop(&vm->stack);
    if (func_value.kind != ValueKindFunc) {
      ERROR("Value is not callable\n");
      return ExecStateExit;
    }

    if (expr->as.func_call.args.len != func_value.as.func.args.len) {
      ERROR("Wrong arguments count: %u, expected %u\n", expr->as.func_call.args.len,
            func_value.as.func.args.len);
      return ExecStateExit;
    }

    for (u32 i = 0; i < expr->as.func_call.args.len; ++i)
      EXECUTE_EXPR(vm, expr->as.func_call.args.items[i], true);

    EXECUTE_FUNC(vm, &func_value.as.func, value_expected);
  } break;

  case IrExprKindVarDef: {
    EXECUTE_EXPR(vm, expr->as.var_def.expr, true);

    Var *prev_var = get_var(vm, expr->as.var_def.name);
    if (prev_var) {
      free_value(&prev_var->value, vm->rc_arena);
      prev_var->value = value_stack_pop(&vm->stack);

      if (value_expected)
        value_stack_push_unit(&vm->stack);

      break;
    }

    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value = value_stack_pop(&vm->stack);
    var.is_global = !vm->is_inside_of_func;

    if (var.is_global)
      DA_APPEND(vm->global_vars, var);
    else
      DA_APPEND(vm->local_vars, var);

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindIf: {
    EXECUTE_EXPR(vm, expr->as._if.cond, true);

    Value cond = value_stack_pop(&vm->stack);
    if (cond.kind != ValueKindBool &&
        cond.kind != ValueKindUnit &&
        cond.kind != ValueKindList) {
      ERROR("Only boolean, unit or list value can be used as a condition\n");
      vm->exit_code = 1;
      return ExecStateExit;
    }

    if (cond.kind != ValueKindUnit &&
        ((cond.kind == ValueKindBool && cond.as._bool) ||
         (cond.kind == ValueKindList && cond.as.list->next))) {
      EXECUTE_BLOCK(vm, &expr->as._if.if_body, value_expected);
      break;
    }

    bool executed_elif = false;

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      EXECUTE_EXPR(vm, expr->as._if.elifs.items[i].cond, true);

      cond = value_stack_pop(&vm->stack);
      if (cond.kind != ValueKindBool &&
          cond.kind != ValueKindUnit &&
          cond.kind != ValueKindList) {
        ERROR("Only boolean, unit or list value can be used as a condition\n");
        vm->exit_code = 1;
        return false;
      }

      if (cond.kind != ValueKindUnit &&
          ((cond.kind == ValueKindBool && cond.as._bool) ||
           (cond.kind == ValueKindList && cond.as.list->next != NULL))) {
        EXECUTE_BLOCK(vm, &expr->as._if.elifs.items[i].body, value_expected);
        executed_elif = true;
        break;
      }
    }

    if (!executed_elif) {
      if (expr->as._if.has_else)
        EXECUTE_BLOCK(vm, &expr->as._if.else_body, value_expected);
      else
        value_stack_push_unit(&vm->stack);
    }
  } break;

  case IrExprKindWhile: {
    while (true) {
      EXECUTE_EXPR(vm, expr->as._while.cond, true);

      Value cond = value_stack_pop(&vm->stack);
      if (cond.kind != ValueKindBool &&
          cond.kind != ValueKindUnit &&
          cond.kind != ValueKindList) {
        ERROR("Only boolean, unit or list value can be used as a condition\n");
        vm->exit_code = 1;
        return ExecStateExit;
      }

      if (cond.kind == ValueKindUnit ||
          ((cond.kind == ValueKindBool && !cond.as._bool) ||
           (cond.kind == ValueKindList && cond.as.list->next == NULL)))
        break;

      EXECUTE_BLOCK(vm, &expr->as._while.body, false);
    }

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindSet: {
    Var *var = get_var(vm, expr->as.set.dest);
    if (!var) {
      ERROR("Symbol "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.set.dest));
      vm->exit_code = 1;
      return ExecStateExit;
    }

    free_value(&var->value, vm->rc_arena);
    EXECUTE_EXPR(vm, expr->as.set.src, true);
    var->value = value_stack_pop(&vm->stack);

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindField: {
    EXECUTE_EXPR(vm, expr->as.field.record, true);

    Value value = value_stack_pop(&vm->stack);
    if (value.kind != ValueKindRecord) {
      ERROR("Only records have fields\n");
      vm->exit_code = 1;
      return ExecStateExit;
    }

    NamedValue *field = NULL;

    for (u32 i = 0; i < value.as.record.len; ++i) {
      NamedValue *temp_field = value.as.record.items + i;

      if (str_eq(temp_field->name, expr->as.field.field)) {
        field = temp_field;
        break;
      }
    }

    if (!field) {
      ERROR("Field `"STR_FMT"` was not found in given record\n",
            STR_ARG(expr->as.field.field));
      return ExecStateExit;
    }

    if (expr->as.field.is_set) {
      free_value(&field->value, vm->rc_arena);

      EXECUTE_EXPR(vm, expr->as.field.expr, true);
      field->value = value_stack_pop(&vm->stack);
      if (value_expected)
        value_stack_push_unit(&vm->stack);
    } else if (value_expected) {
      if (!field) {
        ERROR("Field `"STR_FMT"` was not found in given record\n",
              STR_ARG(expr->as.field.field));
      }

      DA_APPEND(vm->stack, field->value);
    }
  } break;

  case IrExprKindRet: {
    EXECUTE_EXPR(vm, expr->as.ret.expr, true);

    return ExecStateReturn;
  } break;

  case IrExprKindList: {
    if (!value_expected)
      break;
    ListNode *list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    ListNode *list_end = list;

    for (u32 i = 0; i < expr->as.list.content.len; ++i) {
      EXECUTE_EXPR(vm, expr->as.list.content.items[i], true);

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
      return ExecStateContinue;

    Var *var = get_var(vm, expr->as.ident.ident);
    if (var) {
      DA_APPEND(vm->stack, var->value);

      return ExecStateContinue;
    }

    ERROR("Symbol "STR_FMT" was not defined before usage\n",
          STR_ARG(expr->as.ident.ident));
    vm->exit_code = 1;
    return ExecStateExit;
  } break;

  case IrExprKindString: {
    if (value_expected)
      value_stack_push_string(&vm->stack, expr->as.string.lit);
  } break;

  case IrExprKindInt: {
    if (value_expected)
      value_stack_push_int(&vm->stack, expr->as._int._int);
  } break;

  case IrExprKindFloat: {
    if (value_expected)
      value_stack_push_float(&vm->stack, expr->as._float._float);
  } break;

  case IrExprKindBool: {
    if (value_expected)
      value_stack_push_bool(&vm->stack, expr->as._bool._bool);
  } break;

  case IrExprKindLambda: {
    Strs local_names = {0};
    NamedValues catched_values = {0};

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      DA_APPEND(local_names, expr->as.lambda.args.items[i]);

    CATCH_VARS_BLOCK(vm, &local_names, &catched_values, &expr->as.lambda.body);

    if (value_expected) {
      Value func_value = {
        ValueKindFunc,
        {
          .func = {
            expr->as.lambda.args,
            expr->as.lambda.body,
            catched_values,
            expr->as.lambda.intrinsic_name,
          },
        },
      };
      DA_APPEND(vm->stack, func_value);
    }
  } break;

  case IrExprKindRecord: {
    if (!value_expected)
      break;

    Record record = {0};

    for (u32 i = 0; i < expr->as.record.len; ++i) {
      EXECUTE_EXPR(vm, expr->as.record.items[i].expr, true);

      NamedValue field;
      field.name = expr->as.record.items[i].name;
      field.value = value_stack_pop(&vm->stack);

      DA_APPEND(record, field);
    }

    value_stack_push_record(&vm->stack, record);
  } break;

  case IrExprKindSelfCall: {
    for (u32 i = 0; i < expr->as.self_call.args.len; ++i)
      EXECUTE_EXPR(vm, expr->as.self_call.args.items[i], true);

    EXECUTE_FUNC(vm, &vm->current_func_value, value_expected);
  } break;
  }

  return ExecStateContinue;
}

ExecState execute_block(Vm *vm, IrBlock *block, bool value_expected) {
  for (u32 i = 0; i + 1 < block->len; ++i)
    EXECUTE_EXPR(vm, block->items[i], false);

  if (block->len > 0)
    EXECUTE_EXPR(vm, block->items[block->len - 1], value_expected);

  return ExecStateContinue;
}

static void intrinsics_append(Intrinsics *a, Intrinsic *b, u32 b_len) {
  a->cap += b_len;
  a->items = realloc(a->items, a->cap * sizeof(Intrinsic));
  memcpy(a->items + a->len, b, b_len * sizeof(Intrinsic));
  a->len += b_len;
}

u32 execute(Ir *ir, i32 argc, char **argv, RcArena *rc_arena,
            Intrinsics *intrinsics, Value *result_value) {
  Vm vm = {0};
  vm.rc_arena = rc_arena;

  intrinsics_append(intrinsics, base_intrinsics, base_intrinsics_len);
#ifndef NOSYSTEM
  intrinsics_append(intrinsics, system_intrinsics, system_intrinsics_len);
#endif
#ifndef NOIUI
  intrinsics_append(intrinsics, iui_intrinsics, iui_intrinsics_len);
#endif

  vm.intrinsics = *intrinsics;

  vm.args = rc_arena_alloc(rc_arena, sizeof(ListNode));
  ListNode *args_end = vm.args;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = rc_arena_alloc(rc_arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = rc_arena_alloc(rc_arena, sizeof(ListNode));
    new_arg->value = (Value) {
      ValueKindString,
      { .string = { { buffer, len }, (Str *) buffer } },
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

  execute_block(&vm, ir, result_value != NULL);

  if (result_value)
    --vm.stack.len;

  cleanup(&vm);

  return vm.exit_code;
}

void cleanup(Vm *vm) {
  for (u32 i = 0; i < vm->stack.len; ++i)
    free_value(vm->stack.items + i, vm->rc_arena);
  free(vm->stack.items);

  for (u32 i = 0; i < vm->global_vars.len; ++i)
    free_value(&vm->global_vars.items[i].value, vm->rc_arena);
  free(vm->global_vars.items);

  for (u32 i = 0; i < vm->local_vars.len; ++i)
    free_value(&vm->local_vars.items[i].value, vm->rc_arena);
  free(vm->local_vars.items);

  free(vm->intrinsics.items);

  ListNode *arg = vm->args;
  while (arg) {
    ListNode *prev_arg = arg;
    arg = arg->next;
    rc_arena_free(vm->rc_arena, prev_arg);
  }
}
