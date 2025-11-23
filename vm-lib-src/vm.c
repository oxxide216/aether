#include "aether/vm.h"
#include "aether/misc.h"
#include "intrinsics.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG((meta).file_path), (meta).row + 1, (meta).col + 1

#define CATCH_VARS(vm, local_names, catched_values, arena, values, expr) \
  do {                                                                   \
    catch_vars(vm, local_names, catched_values, arena, values, expr);    \
    if (vm->state != ExecStateContinue)                                  \
      return;                                                            \
  } while (0)

#define CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, block) \
  do {                                                                          \
    catch_vars_block(vm, local_names, catched_values, arena, values, block);    \
    if (vm->state != ExecStateContinue)                                         \
      return;                                                                   \
  } while (0)

typedef Da(Str) Strs;

ListNode *list_clone(ListNode *list, Arena *arena, Values *values) {
  if (!list)
    return NULL;

  ListNode *new_list = NULL;
  ListNode **new_list_next = &new_list;
  ListNode *node = list;
  while (node) {
    *new_list_next = arena_alloc(arena, sizeof(ListNode));
    (*new_list_next)->value = value_clone(node->value, arena, values);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  return new_list;
}

Dict dict_clone(Dict *dict, Arena *arena, Values *values) {
  Dict copy = {
    arena_alloc(arena, dict->len * sizeof(IrField)),
    dict->len,
    dict->len,
  };

  for (u32 i = 0; i < copy.len; ++i) {
    copy.items[i].key = value_clone(dict->items[i].key, arena, values);
    copy.items[i].value = value_clone(dict->items[i].value, arena, values);
  }

  return copy;
}

Value *value_unit(Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  value->kind = ValueKindUnit;
  return value;
}

Value *value_list(ListNode *nodes, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  *value = (Value) { ValueKindList, { .list = nodes } };
  return value;
}

Value *value_string(Str string, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  Str new_string = { arena_alloc(arena, string.len), string.len };
  memcpy(new_string.ptr, string.ptr, new_string.len);
  *value = (Value) { ValueKindString, { .string = new_string } };
  return value;
}

Value *value_int(i64 _int, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  *value = (Value) { ValueKindInt, { ._int = _int } };
  return value;
}

Value *value_float(f64 _float, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  *value = (Value) { ValueKindFloat, { ._float = _float } };
  return value;
}

Value *value_bool(bool _bool, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  *value = (Value) { ValueKindBool, { ._bool = _bool } };
  return value;
}

Value *value_dict(Dict dict, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  *value = (Value) { ValueKindDict, { .dict = dict } };
  return value;
}

Value *value_func(Func func, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  *value = (Value) { ValueKindFunc, { .func = func } };
  return value;
}

Value *value_env(Vm vm, Arena *arena, Values *values) {
  Value *value = value_alloc(arena, values);
  Env *env = arena_alloc(arena, sizeof(Env));
  env->vm = vm;
  *value = (Value) { ValueKindEnv, { .env = env } };
  return value;
}

Value *value_alloc(Arena *arena, Values *local_values) {
  Value *value = arena_alloc(arena, sizeof(Value));
  DA_APPEND(*local_values, value);

  return value;
}

Value *value_clone(Value *value, Arena *arena, Values *values) {
  Value *copy = value_alloc(arena, values);
  *copy = *value;

  if (value->kind == ValueKindList) {
    copy->as.list = arena_alloc(arena, sizeof(ListNode));
    copy->as.list->next = list_clone(value->as.list->next, arena, values);
  } else if (value->kind == ValueKindString) {
    copy->as.string.len = value->as.string.len;
    copy->as.string.ptr = arena_alloc(arena, copy->as.string.len);
    memcpy(copy->as.string.ptr, value->as.string.ptr, copy->as.string.len);
  } else if (value->kind == ValueKindDict) {
    copy->as.dict = dict_clone(&value->as.dict, arena, values);
  }

  return copy;
}

void value_free(Value *value) {
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list->next;
    while (node && !node->is_static) {
      ListNode *next_node = node->next;
      value_free(node->value);

      node = next_node;
    }
  } else if (value->kind == ValueKindDict) {
    for (u32 i = 0; i < value->as.dict.len; ++i) {
      value_free(value->as.dict.items[i].key);
      value_free(value->as.dict.items[i].value);
    }
  } else if (value->kind == ValueKindFunc) {
    arena_free(&value->as.func.catched_values_arena);
  } else if (value->kind == ValueKindEnv) {
    if (value->as.env) {
      free(value->as.env->macros.items);
      free(value->as.env->included_files.items);
      vm_destroy(&value->as.env->vm);
    }
  }
}

bool value_eq(Value *a, Value *b) {
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
    return str_eq(a->as.string, b->as.string);
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

static void catch_vars_block(Vm *vm, Strs *local_names,
                             NamedValues *catched_values,
                             Arena *arena, Values *values,
                             IrBlock *block);

static void catch_vars(Vm *vm, Strs *local_names,
                       NamedValues *catched_values,
                       Arena *arena, Values *values,
                       IrExpr *expr) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, &expr->as.block);
  } break;

  case IrExprKindFuncCall: {
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.func_call.func);
    CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, &expr->as.func_call.args);
  } break;

  case IrExprKindVarDef: {
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.var_def.expr);

    DA_APPEND(*local_names, expr->as.var_def.name);
  } break;

  case IrExprKindIf: {
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as._if.cond);
    CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, &expr->as._if.if_body);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i)
      CATCH_VARS_BLOCK(vm, local_names, catched_values, arena,
                       values, &expr->as._if.elifs.items[i].body);

    if (expr->as._if.has_else)
      CATCH_VARS_BLOCK(vm, local_names, catched_values, arena,
                       values, &expr->as._if.else_body);
  } break;

  case IrExprKindWhile: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, &expr->as._while.body);
  } break;

  case IrExprKindSet: {
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.set.src);
  } break;

  case IrExprKindGetAt: {
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.get_at.src);
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.get_at.key);
  } break;

  case IrExprKindSetAt: {
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.set_at.key);
    CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.set_at.value);

    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.set_at.dest, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.set_at.dest);
    if (var && var->kind != VarKindGlobal) {
      NamedValue value = {
        var->name,
        value_clone(var->value, arena, values),
      };
      DA_APPEND(*catched_values, value);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.ret.expr);
  } break;

  case IrExprKindList: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, &expr->as.list.content);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.ident.ident, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.ident.ident);
    if (var && var->kind != VarKindGlobal) {
      NamedValue value = {
        var->name,
        value_clone(var->value, arena, values),
      };
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

    CATCH_VARS_BLOCK(vm, local_names, catched_values, arena, values, &expr->as.lambda.body);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i)
      CATCH_VARS(vm, local_names, catched_values, arena, values, expr->as.dict.items[i].expr);
  } break;

  case IrExprKindSelf: break;
  }
}

static void catch_vars_block(Vm *vm, Strs *local_names,
                             NamedValues *catched_values,
                             Arena *arena, Values *values,
                             IrBlock *block) {
  for (u32 i = 0; i < block->len; ++i)
    CATCH_VARS(vm, local_names, catched_values, arena, values, block->items[i]);
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

Value *execute_func(Vm *vm, Value **args, Func *func, IrMetaData *meta, bool value_expected) {
  if (func->intrinsic_name.len > 0) {
    Intrinsic *intrinsic = get_intrinsic(vm, func->intrinsic_name, func->args.len, args);

    if (!intrinsic) {
      StringBuilder sb = {0};
      sb_push_str(&sb, func->intrinsic_name);
      sb_push(&sb, " [");
      for (u32 i = 0; i < func->args.len; ++i) {
        if (i > 0)
          sb_push_char(&sb, ' ');
        SB_PUSH_VALUE(&sb, args[i], 0, true, vm);
      }
      sb_push_char(&sb, ']');

      Str signature = sb_to_str(sb);

      if (meta)
        PERROR(META_FMT, "Intrinsic `"STR_FMT"` was not found\n",
               META_ARG(*meta), STR_ARG(signature));
      else
        ERROR("Intrinsic `"STR_FMT"` was not found\n",
              STR_ARG(signature));

      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return NULL;
    }

    Value *result = (*intrinsic->func)(vm, args);

    if (vm->state == ExecStateReturn)
      vm->state = ExecStateContinue;

    return result;
  }

  Vars prev_local_vars = vm->local_vars;
  bool prev_is_inside_of_func = vm->is_inside_of_func;
  Func prev_current_func_value = vm->current_func_value;
  Values prev_values = vm->values;
  Arena prev_arena = vm->arena;

  vm->local_vars = (Vars) {0};
  vm->is_inside_of_func = true;
  vm->current_func_value = *func;
  vm->values = (Values) {0};
  vm->arena = (Arena) {0};

  for (u32 i = 0; i < func->args.len; ++i) {
    Var var = {
      func->args.items[i],
      args[i],
      VarKindLocal,
    };

    DA_APPEND(vm->local_vars, var);
  }

  for (u32 i = 0; i < func->catched_values.len; ++i) {
    Var var = {
      func->catched_values_names.items[i].name,
      func->catched_values_names.items[i].value,
      VarKindCatched,
    };

    DA_APPEND(vm->local_vars, var);
  }

  Value *result = execute_block(vm, &func->body, value_expected);

  Value *result_stable = NULL;
  if (value_expected && vm->state == ExecStateContinue)
    result_stable = value_clone(result, &prev_arena, &vm->values);

  if (vm->local_vars.items)
    free(vm->local_vars.items);
  vm->local_vars = prev_local_vars;
  vm->is_inside_of_func = prev_is_inside_of_func;
  vm->current_func_value = prev_current_func_value;

  for (u32 i = 0; i < vm->values.len; ++i)
    value_free(vm->values.items[i]);
  vm->values = prev_values;

  arena_free(&vm->arena);
  vm->arena = prev_arena;

  if (vm->state == ExecStateReturn)
    vm->state = ExecStateContinue;

  return result_stable;
}

Value *execute_expr(Vm *vm, IrExpr *expr, bool value_expected) {
  Value *result = NULL;

  switch (expr->kind) {
  case IrExprKindBlock: {
    EXECUTE_BLOCK(vm, &expr->as.block, value_expected);
  } break;

  case IrExprKindFuncCall: {
    Value *func_value;
    EXECUTE_EXPR_SET(vm, func_value, expr->as.func_call.func, true);

    if (func_value->kind != ValueKindFunc) {
      PERROR(META_FMT, "Value is not callable\n",
             META_ARG(expr->meta));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return NULL;
    }

    if (expr->as.func_call.args.len != func_value->as.func.args.len) {
      PERROR(META_FMT, "Wrong arguments count: %u, expected %u\n",
            META_ARG(expr->meta), expr->as.func_call.args.len,
            func_value->as.func.args.len);
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return NULL;
    }

    Value **func_args = malloc(expr->as.func_call.args.len * sizeof(Value *));

    for (u32 i = 0; i < expr->as.func_call.args.len; ++i) {
      func_args[i] = execute_expr(vm, expr->as.func_call.args.items[i], true);
      if (vm->state != ExecStateContinue) {
        free(func_args);
        return NULL;
      }
    }

    result = execute_func(vm, func_args, &func_value->as.func, &expr->meta, value_expected);
    if (vm->state != ExecStateContinue) {
      free(func_args);
      return NULL;
    }

    free(func_args);
  } break;

  case IrExprKindVarDef: {
    Value *value;
    EXECUTE_EXPR_SET(vm, value, expr->as.var_def.expr, true);

    Var *prev_var = get_var(vm, expr->as.var_def.name);
    if (prev_var) {
      if (prev_var->value != value)
        prev_var->value = value;

      break;
    }

    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value = value;
    var.kind = vm->is_inside_of_func ? VarKindLocal : VarKindGlobal;

    if (var.kind == VarKindGlobal)
      DA_APPEND(vm->global_vars, var);
    else
      DA_APPEND(vm->local_vars, var);
  } break;

  case IrExprKindIf: {
    Value *cond;
    EXECUTE_EXPR_SET(vm, cond, expr->as._if.cond, true);
    if (value_to_bool(cond)) {
      EXECUTE_BLOCK_SET(vm, result, &expr->as._if.if_body, value_expected);
      return result;
    }

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      EXECUTE_EXPR_SET(vm, cond, expr->as._if.elifs.items[i].cond, true);
      if (value_to_bool(cond)) {
        EXECUTE_BLOCK_SET(vm, result, &expr->as._if.elifs.items[i].body, value_expected);
        return result;
      }
    }

    if (expr->as._if.has_else)
      EXECUTE_BLOCK_SET(vm, result, &expr->as._if.else_body, value_expected);
  } break;

  case IrExprKindWhile: {
    while (true) {
      Value *cond;
      EXECUTE_EXPR_SET(vm, cond, expr->as._while.cond, true);
      if (!value_to_bool(cond))
        break;

      EXECUTE_BLOCK(vm, &expr->as._while.body, false);
    }
  } break;

  case IrExprKindSet: {
    Var *dest_var = get_var(vm, expr->as.set.dest);

    if (!dest_var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
        META_ARG(expr->meta), STR_ARG(expr->as.ident.ident));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return NULL;
    }

    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.set.src, true);

    if (dest_var->value == src)
      break;

    dest_var->value = value_clone(src, &vm->arena, &vm->values);

    if (value_expected)
      return value_unit(&vm->arena, &vm->values);
  } break;

  case IrExprKindGetAt: {
    if (!value_expected)
      break;

    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.get_at.src, true);
    Value *key;
    EXECUTE_EXPR_SET(vm, key, expr->as.get_at.key, true);

    if (src->kind == ValueKindList) {
      ListNode *node = src->as.list->next;
      u32 i = 0;
      while (node && i < (u32) key->as._int) {
        node = node->next;
        ++i;
      }

      if (!node) {
        PERROR(META_FMT, "get-at: out of bounds\\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      result = node->value;
    } else if (src->kind == ValueKindString) {
      if ((u32) key->as._int >= src->as.string.len) {
        PERROR(META_FMT, "get-at: out of bounds\\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      Str result_string = src->as.string;
      result_string.ptr += key->as._int;
      result_string.len = 1;

      result = value_string(result_string, &vm->arena, &vm->values);
    } else if (src->kind == ValueKindDict) {
      bool found = false;

      for (u32 i = 0; i < src->as.dict.len; ++i) {
        if (value_eq(src->as.dict.items[i].key, key)) {
          result = src->as.dict.items[i].value;
          found = true;

          break;
        }
      }

      if (!found)
        result = value_unit(&vm->arena, &vm->values);
    } else {
      PERROR(META_FMT, "get-at: source should be list, string or dictionary\\n",
             META_ARG(expr->meta));
      vm->state = ExecStateExit;
      vm->exit_code = 1;
    }
  } break;

  case IrExprKindSetAt: {
    Var *dest_var = get_var(vm, expr->as.set_at.dest);
    if (!dest_var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
            META_ARG(expr->meta), STR_ARG(expr->as.set_at.dest));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return NULL;
    }

    Value *key;
    EXECUTE_EXPR_SET(vm, key, expr->as.set_at.key, true);
    Value *value;
    EXECUTE_EXPR_SET(vm, value, expr->as.set_at.value, true);

    if (dest_var->value->kind == ValueKindList) {
      ListNode *node = dest_var->value->as.list->next;
      u32 i = 0;
      while (node && i < (u32) key->as._int) {
        node = node->next;
        ++i;
      }

      if (!node) {
        PERROR(META_FMT, "set-at: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      node->value = value;
    } else if (dest_var->value->kind == ValueKindString) {
      if ((u32) key->as._int >= dest_var->value->as.string.len) {
        PERROR(META_FMT, "set-at: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      if (value->as.string.len != 1) {
        PERROR(META_FMT, "set-at: string of length 1 can be assigned\\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      dest_var->value->as.string.ptr[key->as._int] = value->as.string.ptr[0];
    } else if (dest_var->value->kind == ValueKindDict) {
      bool found = false;

      for (u32 i = 0; i < dest_var->value->as.dict.len; ++i) {
        if (value_eq(dest_var->value->as.dict.items[i].key, key)) {
          dest_var->value->as.dict.items[i].value = value;
          found = true;

          break;
        }
      }

      if (!found) {
        DictValue dict_value = { key, value };
        DA_APPEND(dest_var->value->as.dict, dict_value);
      }
    } else {
      PERROR(META_FMT, "set-at: destination should be list, string or dictionary\n",
             META_ARG(expr->meta));
      vm->state = ExecStateExit;
      vm->exit_code = 1;
    }

  if (value_expected)
    result = value_unit(&vm->arena, &vm->values);
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      EXECUTE_EXPR_SET(vm, result, expr->as.ret.expr, true);

    vm->state = ExecStateReturn;
  } break;

  case IrExprKindList: {
    if (!value_expected)
      break;

    ListNode *list = arena_alloc(&vm->arena, sizeof(ListNode));
    ListNode *list_end = list;

    for (u32 i = 0; i < expr->as.list.content.len; ++i) {
      ListNode *new_node = arena_alloc(&vm->arena, sizeof(ListNode));
      EXECUTE_EXPR_SET(vm, new_node->value, expr->as.list.content.items[i], true);
      new_node->next = NULL;

      if (list_end) {
        list_end->next = new_node;
        list_end = new_node;
      } else {
        list = new_node;
        list_end = new_node;
      }
    }

    result = value_list(list, &vm->arena, &vm->values);
  } break;

  case IrExprKindIdent: {
    if (!value_expected)
      break;

    Var *var = get_var(vm, expr->as.ident.ident);
    if (!var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
            META_ARG(expr->meta), STR_ARG(expr->as.ident.ident));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return NULL;
    }

    result = value_clone(var->value, &vm->arena, &vm->values);
  } break;

  case IrExprKindString: {
    if (value_expected)
      result = value_string(expr->as.string.lit, &vm->arena, &vm->values);
  } break;

  case IrExprKindInt: {
    if (value_expected)
      result = value_int(expr->as._int._int, &vm->arena, &vm->values);
  } break;

  case IrExprKindFloat: {
    if (value_expected)
      result = value_float(expr->as._float._float, &vm->arena, &vm->values);
  } break;

  case IrExprKindBool: {
    if (value_expected)
      result = value_bool(expr->as._bool._bool, &vm->arena, &vm->values);
  } break;

  case IrExprKindLambda: {
    if (!value_expected)
      break;

    Strs local_names = {0};
    NamedValues catched_values_names = {0};
    Arena arena = {0};
    Values values = {0};

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      DA_APPEND(local_names, expr->as.lambda.args.items[i]);

    catch_vars_block(vm, &local_names, &catched_values_names,
                     &arena, &values, &expr->as.lambda.body);

    Value *func_value = arena_alloc(&vm->arena, sizeof(Value));
    *func_value = (Value) {
      ValueKindFunc,
      {
        .func = {
          expr->as.lambda.args,
          expr->as.lambda.body,
          catched_values_names,
          arena,
          values,
          expr->as.lambda.intrinsic_name,
        },
      },
    };

    free(local_names.items);

    result = func_value;
  } break;

  case IrExprKindDict: {
    if (!value_expected)
      break;

    Dict dict = {0};

    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      DictValue field = {0};
      EXECUTE_EXPR_SET(vm, field.key, expr->as.dict.items[i].key, true);
      EXECUTE_EXPR_SET(vm, field.value, expr->as.dict.items[i].expr, true);

      DA_APPEND(dict, field);
    }

    result = value_dict(dict, &vm->arena, &vm->values);
  } break;

  case IrExprKindSelf: {
    if (value_expected) {
      if (vm->is_inside_of_func)
        return value_func(vm->current_func_value, &vm->arena, &vm->values);
      else
        result = value_unit(&vm->arena, &vm->values);
    }
  } break;
  }

  if (value_expected && !result)
    return value_unit(&vm->arena, &vm->values);
  return result;
}

Value *execute_block(Vm *vm, IrBlock *block, bool value_expected) {
  for (u32 i = 0; i + 1 < block->len; ++i)
    EXECUTE_EXPR(vm, block->items[i], false);

  Value *result = NULL;

  if (block->len > 0)
    EXECUTE_EXPR_SET(vm, result, block->items[block->len - 1], value_expected);
  else if (value_expected)
    result = value_unit(&vm->arena, &vm->values);

  return result;
}

static void intrinsics_append(Intrinsics *a, Intrinsic *b, u32 b_len) {
  a->cap += b_len;
  a->items = realloc(a->items, a->cap * sizeof(Intrinsic));
  memcpy(a->items + a->len, b, b_len * sizeof(Intrinsic));
  a->len += b_len;
}

Vm vm_create(i32 argc, char **argv, Intrinsics *intrinsics) {
  Vm vm = {0};

  intrinsics_append(intrinsics, core_intrinsics, core_intrinsics_len);
  intrinsics_append(intrinsics, math_intrinsics, math_intrinsics_len);
  intrinsics_append(intrinsics, str_intrinsics, str_intrinsics_len);
#ifndef NOSYSTEM
  intrinsics_append(intrinsics, base_intrinsics, base_intrinsics_len);
  intrinsics_append(intrinsics, io_intrinsics, io_intrinsics_len);
  intrinsics_append(intrinsics, path_intrinsics, path_intrinsics_len);
  intrinsics_append(intrinsics, net_intrinsics, net_intrinsics_len);
  intrinsics_append(intrinsics, term_intrinsics, term_intrinsics_len);
  intrinsics_append(intrinsics, system_intrinsics, system_intrinsics_len);
#endif
#ifdef GLASS
  intrinsics_append(intrinsics, glass_intrinsics, glass_intrinsics_len);
#endif

  vm.intrinsics = *intrinsics;

  vm.argc = argc;
  vm.argv = argv;
  vm.args = arena_alloc(&vm.arena, sizeof(ListNode));
  ListNode *args_end = vm.args;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = arena_alloc(&vm.arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = arena_alloc(&vm.arena, sizeof(ListNode));
    new_arg->value = arena_alloc(&vm.arena, sizeof(Value));
    *new_arg->value = (Value) {
      ValueKindString,
      { .string = { buffer, len } },
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

  Value *unit_value = value_alloc(&vm.arena, &vm.values);
  unit_value->kind = ValueKindUnit;
  Var unit_var = { STR_LIT("unit"), unit_value, VarKindGlobal };
  DA_APPEND(vm.global_vars, unit_var);

  return vm;
}

void vm_destroy(Vm *vm) {
  free(vm->global_vars.items);
  free(vm->local_vars.items);
  free(vm->intrinsics.items);
  free(vm->values.items);
  arena_free(&vm->arena);
}
