#include "aether/vm.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"
#include "shl/shl-arena.h"
#include "base/intrinsics.h"
#ifndef NOSYSTEM
#include "system/intrinsics.h"
#endif
#ifndef NOIUI
#include "iui/intrinsics.h"
#endif

typedef Da(Str) Strs;

static Value unit_value = (Value) { ValueKindUnit, {}, 0 };

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
    (*new_list_next)->value = value_clone(rc_arena, node->value);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  return new_list;
}

Dict dict_clone(RcArena *rc_arena, Dict *dict) {
  Dict copy = {
    malloc(dict->len * sizeof(IrField)),
    dict->len,
    dict->len,
  };

  for (u32 i = 0; i < copy.len; ++i)
    copy.items[i].value = value_clone(rc_arena, dict->items[i].value);

  return copy;
}

void value_stack_push_unit(ValueStack *stack) {
  DA_APPEND(*stack, &unit_value);
}

void value_stack_push_list(ValueStack *stack, RcArena *rc_arena, ListNode *nodes) {
  Value *value = rc_arena_alloc(rc_arena, sizeof(Value));
  *value = (Value) { ValueKindList, { .list = nodes }, 0 };
  DA_APPEND(*stack, value);
}

void value_stack_push_string(ValueStack *stack, RcArena *rc_arena, Str string) {
  Value *value = rc_arena_alloc(rc_arena, sizeof(Value));
  *value = (Value) {
    ValueKindString,
    { .string = { { string.ptr, string.len }, (Str *) string.ptr } },
    0,
  };
  DA_APPEND(*stack, value);
}

void value_stack_push_int(ValueStack *stack, RcArena *rc_arena, i64 _int) {
  Value *value = rc_arena_alloc(rc_arena, sizeof(Value));
  *value = (Value) { ValueKindInt, { ._int = _int }, 0 };
  DA_APPEND(*stack, value);
}

void value_stack_push_float(ValueStack *stack, RcArena *rc_arena, f64 _float) {
  Value *value = rc_arena_alloc(rc_arena, sizeof(Value));
  *value = (Value) { ValueKindFloat, { ._float = _float }, 0 };
  DA_APPEND(*stack, value);
}

void value_stack_push_bool(ValueStack *stack, RcArena *rc_arena, bool _bool) {
  Value *value = rc_arena_alloc(rc_arena, sizeof(Value));
  *value = (Value) { ValueKindBool, { ._bool = _bool }, 0 };
  DA_APPEND(*stack, value);
}

void value_stack_push_dict(ValueStack *stack, RcArena *rc_arena, Dict dict) {
  Value *value = rc_arena_alloc(rc_arena, sizeof(Value));
  *value = (Value) { ValueKindDict, { .dict = dict }, 0 };
  DA_APPEND(*stack, value);
}

Value *value_stack_pop(ValueStack *stack) {
  if (stack->len > 0) {
    ++stack->items[stack->len - 1]->refs_count;
    return stack->items[--stack->len];
  }

  return &unit_value;
}

Value *value_clone(RcArena *rc_arena, Value *value) {
  Value *copy = rc_arena_alloc(rc_arena, sizeof(Value));
  *copy = *value;
  copy->refs_count = 0;

  if (value->kind == ValueKindList) {
    copy->as.list = list_clone(rc_arena, value->as.list);
  } else if (value->kind == ValueKindString) {
    copy->as.string.str.ptr = rc_arena_alloc(rc_arena, value->as.string.str.len);
    copy->as.string.str.len = value->as.string.str.len;
    memcpy(copy->as.string.str.ptr, value->as.string.str.ptr, copy->as.string.str.len);
    copy->as.string.begin = (Str *)(copy->as.string.str.ptr +
                                    (u64) (char *) value->as.string.begin -
                                    (u64) value->as.string.str.ptr);
  } else if (value->kind == ValueKindDict) {
    for (u32 i = 0; i < value->as.dict.len; ++i)
      value_free(value->as.dict.items[i].value, rc_arena);
  }

  return copy;
}

void value_free(Value *value, RcArena *rc_arena) {
  if (value->refs_count > 0)
    --value->refs_count;

  if (value->refs_count == 0) {
    if (value->kind == ValueKindList) {
      ListNode *node = value->as.list;
      while (node && !node->is_static) {
        ListNode *next_node = node->next;
        rc_arena_free(rc_arena, node);
        node = next_node;
      }
    } else if (value->kind == ValueKindString) {
      rc_arena_free(rc_arena, value->as.string.begin);
    } else if (value->kind == ValueKindDict) {
      for (u32 i = 0; i < value->as.dict.len; ++i)
        value_free(value->as.dict.items[i].value, rc_arena);
    }

    if (value->kind != ValueKindUnit)
      rc_arena_free(rc_arena, value);
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

  case IrExprKindGet: {
    CATCH_VARS(vm, local_names, catched_values, expr->as.get.key);
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

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i)
      CATCH_VARS(vm, local_names, catched_values, expr->as.dict.items[i].expr);
  } break;

  case IrExprKindSelfCall: {
    for (u32 i = 0; i < expr->as.self_call.args.len; ++i)
      CATCH_VARS(vm, local_names, catched_values, expr->as.self_call.args.items[i]);
  } break;

  case IrExprKindSetIn: {
    for (u32 i = 0; i < expr->as.set_in.fields.len; ++i) {
      CATCH_VARS(vm, local_names, catched_values, expr->as.set_in.fields.items[i].key);
      CATCH_VARS(vm, local_names, catched_values, expr->as.set_in.fields.items[i].expr);
    }
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

static bool value_eq(Value *a, Value *b) {
  if (a->kind != b->kind)
    return false;

  switch (a->kind) {
  case ValueKindUnit: {
    return true;
  }

  case ValueKindList: {
    ListNode *a_node = a->as.list->next;
    ListNode *b_node = b->as.list->next;

    while (a_node && b_node) {
      if (!value_eq(a_node->value, b_node->value))
        return false;

      a_node = a_node->next;
      b_node = b_node->next;
    }

    return a_node == NULL && b_node == NULL;
  }

  case ValueKindString: {
    return str_eq(a->as.string.str, b->as.string.str);
  }

  case ValueKindInt: {
    return a->as._int == b->as._int;
  } break;

  case ValueKindFloat: {
    return a->as._float == b->as._float;
  }

  case ValueKindBool: {
    return a->as._bool == b->as._bool;
  }

  case ValueKindDict: {
    if (a->as.dict.len != b->as.dict.len)
      return false;

    for (u32 i = 0; i < a->as.dict.len; ++i)
      if (!value_eq(a->as.dict.items[i].key, b->as.dict.items[i].key) ||
          !value_eq(a->as.dict.items[i].value, b->as.dict.items[i].value))
        return false;

    return true;
  }

  case ValueKindFunc: {
    if (a->as.func.intrinsic_name.len > 0)
      return str_eq(a->as.func.intrinsic_name, b->as.func.intrinsic_name);

    return false;
  }

  default: {
    return false;
  }
  }
}

bool value_list_matches_kinds(u32 len, Value **values, ValueKind *kinds) {
  for (u32 i = 0; i < len; ++i)
    if (values[i]->kind != kinds[i] && kinds[i] != ValueKindUnit)
      return false;

  return true;
}

static Intrinsic *get_intrinsic(Vm *vm, Str name, u32 args_count, Value **args) {
  for (u32 i = 0; i < vm->intrinsics.len; ++i) {
    Intrinsic *intrinsic = vm->intrinsics.items + i;

    if (str_eq(intrinsic->name, name) &&
        intrinsic->args_count == args_count &&
        value_list_matches_kinds(args_count, args, intrinsic->arg_kinds)) {
      return intrinsic;
    }
  }

  return NULL;
}

ExecState execute_func(Vm *vm, Func *func, bool value_expected) {
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
    value_free(vm->stack.items[i], vm->rc_arena);

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

    Value *func_value = value_stack_pop(&vm->stack);
    if (func_value->kind != ValueKindFunc) {
      ERROR("Value is not callable\n");
      return ExecStateExit;
    }

    if (expr->as.func_call.args.len != func_value->as.func.args.len) {
      ERROR("Wrong arguments count: %u, expected %u\n",
            expr->as.func_call.args.len,
            func_value->as.func.args.len);
      return ExecStateExit;
    }

    for (u32 i = 0; i < expr->as.func_call.args.len; ++i)
      EXECUTE_EXPR(vm, expr->as.func_call.args.items[i], true);

    EXECUTE_FUNC(vm, &func_value->as.func, value_expected);
  } break;

  case IrExprKindVarDef: {
    EXECUTE_EXPR(vm, expr->as.var_def.expr, true);

    Var *prev_var = get_var(vm, expr->as.var_def.name);
    if (prev_var) {
      value_free(prev_var->value, vm->rc_arena);
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

    Value *cond = value_stack_pop(&vm->stack);
    if (cond->kind != ValueKindBool &&
        cond->kind != ValueKindUnit &&
        cond->kind != ValueKindList) {
      ERROR("Only boolean, unit or list value can be used as a condition\n");
      vm->exit_code = 1;
      return ExecStateExit;
    }

    if (cond->kind != ValueKindUnit &&
        ((cond->kind == ValueKindBool && cond->as._bool) ||
         (cond->kind == ValueKindList && cond->as.list->next))) {
      EXECUTE_BLOCK(vm, &expr->as._if.if_body, value_expected);
      break;
    }

    bool executed_elif = false;

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      EXECUTE_EXPR(vm, expr->as._if.elifs.items[i].cond, true);

      cond = value_stack_pop(&vm->stack);
      if (cond->kind != ValueKindBool &&
          cond->kind != ValueKindUnit &&
          cond->kind != ValueKindList) {
        ERROR("Only boolean, unit or list value can be used as a condition\n");
        vm->exit_code = 1;
        return false;
      }

      if (cond->kind != ValueKindUnit &&
          ((cond->kind == ValueKindBool && cond->as._bool) ||
           (cond->kind == ValueKindList && cond->as.list->next != NULL))) {
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

      Value *cond = value_stack_pop(&vm->stack);
      if (cond->kind != ValueKindBool &&
          cond->kind != ValueKindUnit &&
          cond->kind != ValueKindList) {
        ERROR("Only boolean, unit or list value can be used as a condition\n");
        vm->exit_code = 1;
        return ExecStateExit;
      }

      if (cond->kind == ValueKindUnit ||
          ((cond->kind == ValueKindBool && !cond->as._bool) ||
           (cond->kind == ValueKindList && cond->as.list->next == NULL)))
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

    Value *prev_value = var->value;

    EXECUTE_EXPR(vm, expr->as.set.src, true);
    var->value = value_stack_pop(&vm->stack);

    value_free(prev_value, vm->rc_arena);

    if (var->value->refs_count > 1)
      var->value = value_clone(vm->rc_arena, var->value);

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindGet: {
    Var *var = get_var(vm, expr->as.get.src);
    if (!var) {
      ERROR("Symbol "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.set.dest));
      vm->exit_code = 1;
      return ExecStateExit;
    }

    if (var->value->kind != ValueKindList &&
        var->value->kind != ValueKindDict) {
      ERROR("Only lists and dictionaries can be indexed\n");
      vm->exit_code = 1;
      return ExecStateExit;
    }

    EXECUTE_EXPR(vm, expr->as.get.key, true);
    Value *key = value_stack_pop(&vm->stack);

    if (var->value->kind == ValueKindList) {
      if (key->kind != ValueKindInt) {
        ERROR("Lists can be indexed only with integers\n");
        vm->exit_code = 1;
        return ExecStateExit;
      }

      ListNode *node = key->as.list->next;
      u32 i = 0;
      while (node && i < key->as._int) {
        node = node->next;
        ++i;
      }

      if (!node || i < key->as._int) {
        ERROR("Out of bounds\n");
        vm->exit_code = 1;
        return ExecStateExit;
      }

      DA_APPEND(vm->stack, node->value);
    } else if (var->value->kind == ValueKindDict) {
      bool assigned = false;

      for (u32 i = 0; i < var->value->as.dict.len; ++i) {
        DictValue *dict_value = var->value->as.dict.items + i;

        if (value_eq(dict_value->key, key)) {
          DA_APPEND(vm->stack, dict_value->value);

          assigned = true;

          break;
        }
      }

      if (!assigned)
        value_stack_push_unit(&vm->stack);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
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

    value_stack_push_list(&vm->stack, vm->rc_arena, list);
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
      value_stack_push_string(&vm->stack, vm->rc_arena, expr->as.string.lit);
  } break;

  case IrExprKindInt: {
    if (value_expected)
      value_stack_push_int(&vm->stack, vm->rc_arena, expr->as._int._int);
  } break;

  case IrExprKindFloat: {
    if (value_expected)
      value_stack_push_float(&vm->stack, vm->rc_arena, expr->as._float._float);
  } break;

  case IrExprKindBool: {
    if (value_expected)
      value_stack_push_bool(&vm->stack, vm->rc_arena, expr->as._bool._bool);
  } break;

  case IrExprKindLambda: {
    Strs local_names = {0};
    NamedValues catched_values = {0};

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      DA_APPEND(local_names, expr->as.lambda.args.items[i]);

    CATCH_VARS_BLOCK(vm, &local_names, &catched_values, &expr->as.lambda.body);

    if (value_expected) {
      Value *func_value = rc_arena_alloc(vm->rc_arena, sizeof(Value));
      *func_value = (Value) {
        ValueKindFunc,
        {
          .func = {
            expr->as.lambda.args,
            expr->as.lambda.body,
            catched_values,
            expr->as.lambda.intrinsic_name,
          },
        },
        0,
      };
      DA_APPEND(vm->stack, func_value);
    }
  } break;

  case IrExprKindDict: {
    if (!value_expected)
      break;

    Dict dict = {0};

    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      EXECUTE_EXPR(vm, expr->as.dict.items[i].expr, true);
      EXECUTE_EXPR(vm, expr->as.dict.items[i].key, true);

      DictValue field;
      field.key = value_stack_pop(&vm->stack);
      field.value = value_stack_pop(&vm->stack);

      DA_APPEND(dict, field);
    }

    value_stack_push_dict(&vm->stack, vm->rc_arena, dict);
  } break;

  case IrExprKindSelfCall: {
    for (u32 i = 0; i < expr->as.self_call.args.len; ++i)
      EXECUTE_EXPR(vm, expr->as.self_call.args.items[i], true);

    EXECUTE_FUNC(vm, &vm->current_func_value, value_expected);
  } break;

  case IrExprKindSetIn: {
    Var *var = get_var(vm, expr->as.set_in.dict);
    if (!var) {
      ERROR("Symbol "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.set.dest));
      vm->exit_code = 1;
      return ExecStateExit;
    }

    if (var->value->kind != ValueKindDict) {
      ERROR("Only dictionaries have fields\n");
      vm->exit_code = 1;
      return ExecStateExit;
    }

    Dict *dict = &var->value->as.dict;

    for (u32 i = 0; i < expr->as.set_in.fields.len; ++i) {
      EXECUTE_EXPR(vm, expr->as.set_in.fields.items[i].expr, true);
      EXECUTE_EXPR(vm, expr->as.set_in.fields.items[i].key, true);

      DictValue field;
      field.key = value_stack_pop(&vm->stack);
      field.value = value_stack_pop(&vm->stack);

      bool found_field = false;

      for (u32 j = 0; j < dict->len; ++j) {
        if (value_eq(dict->items[j].key, field.key)) {
          Value *prev_value = dict->items[j].value;

          dict->items[j].value = field.value;

          value_free(prev_value, vm->rc_arena);

          Value **field_value = &dict->items[j].value;
          if ((*field_value)->refs_count > 1)
            *field_value = value_clone(vm->rc_arena, *field_value);

          found_field = true;

          break;
        }
      }

      if (!found_field)
        DA_APPEND(*dict, field);
    }

    if (value_expected)
      value_stack_push_unit(&vm->stack);
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
            Intrinsics *intrinsics, Value **result_value) {
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
    new_arg->value = rc_arena_alloc(rc_arena, sizeof(Value));
    *new_arg->value = (Value) {
      ValueKindString,
      { .string = { { buffer, len }, (Str *) buffer } },
      0,
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

  cleanup(&vm);

  return vm.exit_code;
}

void cleanup(Vm *vm) {
  for (u32 i = 0; i < vm->stack.len; ++i)
    value_free(vm->stack.items[i], vm->rc_arena);
  free(vm->stack.items);

  for (u32 i = 0; i < vm->global_vars.len; ++i)
    value_free(vm->global_vars.items[i].value, vm->rc_arena);
  free(vm->global_vars.items);

  for (u32 i = 0; i < vm->local_vars.len; ++i)
    value_free(vm->local_vars.items[i].value, vm->rc_arena);
  free(vm->local_vars.items);

  free(vm->intrinsics.items);

  ListNode *arg = vm->args;
  while (arg) {
    ListNode *prev_arg = arg;
    arg = arg->next;
    rc_arena_free(vm->rc_arena, prev_arg);
  }
}
