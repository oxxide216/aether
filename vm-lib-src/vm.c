#include "aether/vm.h"
#include "aether/misc.h"
#include "intrinsics.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define WHILE_FRAME_LENGTH 100

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG((meta).file_path), (meta).row + 1, (meta).col + 1

#define CATCH_VARS(vm, local_names, catched_values, frame, expr) \
  do {                                                           \
    catch_vars(vm, local_names, catched_values, frame, expr);    \
    if (vm->state != ExecStateContinue)                          \
      return;                                                    \
  } while (0)

#define CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, block) \
  do {                                                                  \
    catch_vars_block(vm, local_names, catched_values, frame, block);    \
    if (vm->state != ExecStateContinue)                                 \
      return;                                                           \
  } while (0)

typedef Da(Str) Strs;

ListNode *list_clone(ListNode *nodes, StackFrame *frame, u32 frame_index) {
  if (!nodes)
    return NULL;

  ListNode *new_list = NULL;
  ListNode **new_list_next = &new_list;
  ListNode *node = nodes;
  while (node) {
    *new_list_next = arena_alloc(&frame->arena, sizeof(ListNode));
    (*new_list_next)->value = value_clone(node->value, frame, frame_index);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  return new_list;
}

Dict dict_clone(Dict *dict, StackFrame *frame, u32 frame_index) {
  Dict copy = {
    arena_alloc(&frame->arena, dict->len * sizeof(DictValue)),
    dict->len,
    dict->len,
  };

  for (u32 i = 0; i < copy.len; ++i) {
    copy.items[i].key = value_clone(dict->items[i].key, frame, frame_index);
    copy.items[i].value = value_clone(dict->items[i].value, frame, frame_index);
  }

  return copy;
}

Value *value_unit(StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindUnit, {}, frame_index };
  return value;
}

Value *value_list(ListNode *nodes, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindList, { .list = nodes }, frame_index };
  return value;
}

Value *value_string(Str string, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  Str new_string = { arena_alloc(&frame->arena, string.len), string.len };
  memcpy(new_string.ptr, string.ptr, new_string.len);
  *value = (Value) { ValueKindString, { .string = new_string }, frame_index };
  return value;
}

Value *value_int(i64 _int, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindInt, { ._int = _int }, frame_index };
  return value;
}

Value *value_float(f64 _float, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindFloat, { ._float = _float }, frame_index };
  return value;
}

Value *value_bool(bool _bool, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindBool, { ._bool = _bool }, frame_index };
  return value;
}

Value *value_dict(Dict dict, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindDict, { .dict = dict }, frame_index };
  return value;
}

Value *value_func(Func func, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindFunc, { .func = func }, frame_index };
  return value;
}

Value *value_env(Vm vm, StackFrame *frame, u32 frame_index) {
  Value *value = value_alloc(frame);
  Env *env = malloc(sizeof(Env));
  *env = (Env) {0};
  env->vm = vm;
  env->refs_count = 1;
  *value = (Value) { ValueKindEnv, { .env = env }, frame_index };
  return value;
}

Value *value_alloc(StackFrame *frame) {
  Value *value = arena_alloc(&frame->arena, sizeof(Value));
  DA_APPEND(frame->values, value);

  return value;
}

Value *value_clone(Value *value, StackFrame *frame, u32 frame_index) {
  Value *copy = value_alloc(frame);
  *copy = *value;
  copy->frame_index = frame_index;

  if (value->kind == ValueKindList) {
    copy->as.list = arena_alloc(&frame->arena, sizeof(ListNode));
    copy->as.list->next = list_clone(value->as.list->next, frame, frame_index);
  } else if (value->kind == ValueKindString) {
    copy->as.string.len = value->as.string.len;
    copy->as.string.ptr = arena_alloc(&frame->arena, copy->as.string.len);
    memcpy(copy->as.string.ptr, value->as.string.ptr, copy->as.string.len);
  } else if (value->kind == ValueKindDict) {
    copy->as.dict = dict_clone(&value->as.dict, frame, frame_index);
  } else if (value->kind == ValueKindEnv) {
    ++copy->as.env->refs_count;
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
    arena_free(&value->as.func.catched_frame.arena);
  } else if (value->kind == ValueKindEnv) {
    if (--value->as.env->refs_count == 0) {
      if (value->as.env->macros.items)
        free(value->as.env->macros.items);
      if (value->as.env->included_files.items)
        free(value->as.env->included_files.items);
      vm_destroy(&value->as.env->vm);
      arena_free(&value->as.env->arena);
      free(value->as.env);
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
  u32 depth = 0;

  for (u32 i = vm->current_frame_index + 1;
       i > vm->current_frame_index - depth;
       --i) {
    StackFrame *frame = vm->frames.items + i - 1;

    for (u32 j = frame->vars.len; j > 0; --j)
      if (str_eq(frame->vars.items[j - 1].name, name))
        return frame->vars.items + j - 1;

    if (frame->can_lookup_through)
      ++depth;
  }

  for (u32 j = vm->global_vars.len; j > 0; --j)
    if (str_eq(vm->global_vars.items[j - 1].name, name))
      return vm->global_vars.items + j - 1;

  return NULL;
}

static void catch_vars_block(Vm *vm, Strs *local_names,
                             NamedValues *catched_values,
                             StackFrame *frame,
                             IrBlock *block);

static void catch_vars(Vm *vm, Strs *local_names,
                       NamedValues *catched_values,
                       StackFrame *frame,
                       IrExpr *expr) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as.block);
  } break;

  case IrExprKindFuncCall: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.func_call.func);
    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as.func_call.args);
  } break;

  case IrExprKindVarDef: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.var_def.expr);

    DA_APPEND(*local_names, expr->as.var_def.name);
  } break;

  case IrExprKindIf: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as._if.cond);
    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as._if.if_body);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i)
      CATCH_VARS_BLOCK(vm, local_names, catched_values, frame,
                       &expr->as._if.elifs.items[i].body);

    if (expr->as._if.has_else)
      CATCH_VARS_BLOCK(vm, local_names, catched_values, frame,
                       &expr->as._if.else_body);
  } break;

  case IrExprKindWhile: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as._while.body);
  } break;

  case IrExprKindSet: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.set.src);
  } break;

  case IrExprKindGetAt: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.get_at.src);
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.get_at.key);
  } break;

  case IrExprKindSetAt: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.set_at.key);
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.set_at.value);

    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.set_at.dest, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.set_at.dest);
    if (var && var->kind != VarKindGlobal) {
      NamedValue value = {
        var->name,
        value_clone(var->value, frame, 0),
      };
      DA_APPEND(*catched_values, value);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.ret.expr);
  } break;

  case IrExprKindList: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as.list.content);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.ident.ident, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.ident.ident);
    if (var && var->kind != VarKindGlobal) {
      NamedValue value = {
        var->name,
        value_clone(var->value, frame, 0),
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

    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as.lambda.body);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.dict.items[i].key);
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.dict.items[i].expr);
    }
  } break;

  case IrExprKindMatch: {
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.match.src);

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.match.cases.items[i].pattern);
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.match.cases.items[i].expr);
    }
  } break;
  }
}

static void catch_vars_block(Vm *vm, Strs *local_names,
                             NamedValues *catched_values,
                             StackFrame *frame,
                             IrBlock *block) {
  for (u32 i = 0; i < block->len; ++i)
    CATCH_VARS(vm, local_names, catched_values, frame, block->items[i]);
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

      free(sb.buffer);

      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

    Value *result = (*intrinsic->func)(vm, args);

    if (vm->state == ExecStateReturn)
      vm->state = ExecStateContinue;

    return result;
  }

  bool prev_is_inside_of_func = vm->is_inside_of_func;
  Func prev_current_func_value = vm->current_func_value;

  vm->is_inside_of_func = true;
  vm->current_func_value = *func;

  begin_frame(vm);

  StackFrame *frame = vm_get_frame(vm);

  for (u32 i = 0; i < func->args.len; ++i) {
    Var var = {
      func->args.items[i],
      args[i],
      VarKindLocal,
    };

    DA_APPEND(frame->vars, var);
  }

  for (u32 i = 0; i < func->catched_frame.values.len; ++i) {
    Var var = {
      func->catched_values_names.items[i].name,
      func->catched_values_names.items[i].value,
      VarKindCatched,
    };

    DA_APPEND(frame->vars, var);
  }

  Value *result = execute_block(vm, &func->body, value_expected);

  if (vm->state == ExecStateReturn)
    vm->state = ExecStateContinue;

  frame = vm_get_frame(vm);

  Value *result_stable = NULL;
  if (value_expected && vm->state == ExecStateContinue)
    result_stable = value_clone(result, frame - 1, vm->current_frame_index - 1);
  else
    result_stable = value_unit(frame - 1, vm->current_frame_index - 1);

  end_frame(vm);

  vm->is_inside_of_func = prev_is_inside_of_func;
  vm->current_func_value = prev_current_func_value;

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

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

    if (expr->as.func_call.args.len != func_value->as.func.args.len) {
      PERROR(META_FMT, "Wrong arguments count: %u, expected %u\n",
            META_ARG(expr->meta), expr->as.func_call.args.len,
            func_value->as.func.args.len);
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

    Value **func_args = malloc(expr->as.func_call.args.len * sizeof(Value *));

    for (u32 i = 0; i < expr->as.func_call.args.len; ++i) {
      func_args[i] = execute_expr(vm, expr->as.func_call.args.items[i], true);
      if (vm->state != ExecStateContinue) {
        free(func_args);
        return value_unit(vm_get_frame(vm), vm->current_frame_index);
      }
    }

    result = execute_func(vm, func_args, &func_value->as.func, &expr->meta, value_expected);
    if (vm->state != ExecStateContinue) {
      Str name = STR_LIT("<lambda>");
      if (expr->as.func_call.func->kind == IrExprKindIdent)
        name = expr->as.func_call.func->as.ident.ident;

      INFO("Trace: "META_FMT STR_FMT"\n", META_ARG(expr->meta), STR_ARG(name));

      free(func_args);
      return value_unit(vm_get_frame(vm), vm->current_frame_index);
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
      DA_APPEND(vm_get_frame(vm)->vars, var);
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
    begin_frame(vm);
    vm_get_frame(vm)->can_lookup_through = true;

    u32 i = 0;

    while (true) {
      Value *cond;
      EXECUTE_EXPR_SET(vm, cond, expr->as._while.cond, true);
      if (!value_to_bool(cond))
        break;

      EXECUTE_BLOCK(vm, &expr->as._while.body, false);

      if (i++ == WHILE_FRAME_LENGTH) {
        end_frame(vm);
        begin_frame(vm);

        i = 0;
      }
    }

    vm_get_frame(vm)->can_lookup_through = false;
    end_frame(vm);
  } break;

  case IrExprKindSet: {
    Var *dest_var = get_var(vm, expr->as.set.dest);

    if (!dest_var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
        META_ARG(expr->meta), STR_ARG(expr->as.ident.ident));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.set.src, true);

    if (dest_var->value == src)
      break;

    dest_var->value = value_clone(src, vm->frames.items + dest_var->value->frame_index,
                                  dest_var->value->frame_index);

    if (value_expected)
      return value_unit(vm_get_frame(vm), vm->current_frame_index);
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
        PERROR(META_FMT, "get: out of bounds\\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      result = node->value;
    } else if (src->kind == ValueKindString) {
      if ((u32) key->as._int >= src->as.string.len) {
        PERROR(META_FMT, "get: out of bounds\\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;
      }

      Str result_string = src->as.string;
      result_string.ptr += key->as._int;
      result_string.len = 1;

      result = value_string(result_string, vm_get_frame(vm), vm->current_frame_index);
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
        result = value_unit(vm_get_frame(vm), vm->current_frame_index);
    } else {
      PERROR(META_FMT, "get: source should be list, string or dictionary\n",
             META_ARG(expr->meta));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }
  } break;

  case IrExprKindSetAt: {
    Var *dest_var = get_var(vm, expr->as.set_at.dest);
    if (!dest_var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
            META_ARG(expr->meta), STR_ARG(expr->as.set_at.dest));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

    Value *key;
    EXECUTE_EXPR_SET(vm, key, expr->as.set_at.key, true);
    Value *value;
    EXECUTE_EXPR_SET(vm, value, expr->as.set_at.value, true);

    key = value_clone(key, vm->frames.items + dest_var->value->frame_index,
                      dest_var->value->frame_index);
    value = value_clone(value, vm->frames.items + dest_var->value->frame_index,
                        dest_var->value->frame_index);

    if (dest_var->value->kind == ValueKindList) {
      if (key->kind != ValueKindInt) {
        PERROR(META_FMT, "set: only integer can be used as an array index\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm_get_frame(vm), vm->current_frame_index);
      }

      ListNode *node = dest_var->value->as.list->next;
      u32 i = 0;
      while (node && i < (u32) key->as._int) {
        node = node->next;
        ++i;
      }

      if (!node) {
        PERROR(META_FMT, "set: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm_get_frame(vm), vm->current_frame_index);
      }

      node->value = value;
    } else if (dest_var->value->kind == ValueKindString) {
      if (key->kind != ValueKindInt) {
        PERROR(META_FMT, "set: only integer can be used as a string index\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm_get_frame(vm), vm->current_frame_index);
      }

      if ((u32) key->as._int >= dest_var->value->as.string.len) {
        PERROR(META_FMT, "set: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm_get_frame(vm), vm->current_frame_index);
      }

      if (value->as.string.len != 1) {
        PERROR(META_FMT, "set: only string of length 1 can be assigned\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm_get_frame(vm), vm->current_frame_index);
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

        if (dest_var->value->as.dict.cap == dest_var->value->as.dict.len) {
          if (dest_var->value->as.dict.cap == 0)
            dest_var->value->as.dict.cap = 1;
          else
            dest_var->value->as.dict.cap *= 2;
          DictValue *new_items =
            arena_alloc(&vm_get_frame(vm)->arena, dest_var->value->as.dict.cap * sizeof(DictValue));
          memcpy(new_items, dest_var->value->as.dict.items,
                 dest_var->value->as.dict.len * sizeof(DictValue));
          dest_var->value->as.dict.items = new_items;
        }

        dest_var->value->as.dict.items[dest_var->value->as.dict.len++] = dict_value;
      }
    } else {
      PERROR(META_FMT, "set: destination should be list, string or dictionary\n",
             META_ARG(expr->meta));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

  if (value_expected)
    result = value_unit(vm_get_frame(vm), vm->current_frame_index);
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      EXECUTE_EXPR_SET(vm, result, expr->as.ret.expr, true);

    vm->state = ExecStateReturn;
  } break;

  case IrExprKindList: {
    if (!value_expected)
      break;

    ListNode *list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    ListNode *list_end = list;

    for (u32 i = 0; i < expr->as.list.content.len; ++i) {
      ListNode *new_node = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
      EXECUTE_EXPR_SET(vm, new_node->value, expr->as.list.content.items[i], true);
      new_node->next = NULL;;

      if (list_end) {
        list_end->next = new_node;
        list_end = new_node;
      } else {
        list = new_node;
        list_end = new_node;
      }
    }

    result = value_list(list, vm_get_frame(vm), vm->current_frame_index);
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

      return value_unit(vm_get_frame(vm), vm->current_frame_index);
    }

    result = value_clone(var->value, vm_get_frame(vm),
                         vm->current_frame_index);
  } break;

  case IrExprKindString: {
    if (value_expected)
      result = value_string(expr->as.string.lit, vm_get_frame(vm),
                            vm->current_frame_index);
  } break;

  case IrExprKindInt: {
    if (value_expected)
      result = value_int(expr->as._int._int, vm_get_frame(vm),
                         vm->current_frame_index);
  } break;

  case IrExprKindFloat: {
    if (value_expected)
      result = value_float(expr->as._float._float, vm_get_frame(vm),
                           vm->current_frame_index);
  } break;

  case IrExprKindBool: {
    if (value_expected)
      result = value_bool(expr->as._bool._bool, vm_get_frame(vm),
                          vm->current_frame_index);
  } break;

  case IrExprKindLambda: {
    if (!value_expected)
      break;

    Strs local_names = {0};
    NamedValues catched_values_names = {0};
    StackFrame frame = {0};

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      DA_APPEND(local_names, expr->as.lambda.args.items[i]);

    catch_vars_block(vm, &local_names, &catched_values_names,
                     &frame, &expr->as.lambda.body);

    Value *func_value = arena_alloc(&vm_get_frame(vm)->arena, sizeof(Value));
    *func_value = (Value) {
      ValueKindFunc,
      {
        .func = {
          expr->as.lambda.args,
          expr->as.lambda.body,
          catched_values_names,
          frame,
          expr->as.lambda.intrinsic_name,
        },
      },
      vm->current_frame_index,
    };

    free(local_names.items);

    result = func_value;
  } break;

  case IrExprKindDict: {
    if (!value_expected)
      break;

    Dict dict = {0};
    dict.len = expr->as.dict.len;
    dict.cap = dict.len;
    dict.items = arena_alloc(&vm_get_frame(vm)->arena, dict.cap * sizeof(DictValue));

    for (u32 i = 0; i < dict.len; ++i) {
      DictValue field = {0};
      EXECUTE_EXPR_SET(vm, field.key, expr->as.dict.items[i].key, true);
      EXECUTE_EXPR_SET(vm, field.value, expr->as.dict.items[i].expr, true);

      dict.items[i] = field;
    }

    result = value_dict(dict, vm_get_frame(vm), vm->current_frame_index);
  } break;

  case IrExprKindMatch: {
    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.match.src, true);

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      Value *pattern;
      EXECUTE_EXPR_SET(vm, pattern, expr->as.match.cases.items[i].pattern, true);

      if (value_eq(pattern, src)) {
        EXECUTE_EXPR_SET(vm, result, expr->as.match.cases.items[i].expr, value_expected);
        break;
      }
    }
  } break;
  }

  if (value_expected && !result)
    return value_unit(vm_get_frame(vm), vm->current_frame_index);
  return result;
}

Value *execute_block(Vm *vm, IrBlock *block, bool value_expected) {
  for (u32 i = 0; i + 1 < block->len; ++i)
    EXECUTE_EXPR(vm, block->items[i], false);

  Value *result = NULL;

  if (block->len > 0)
    EXECUTE_EXPR_SET(vm, result, block->items[block->len - 1], value_expected);
  else if (value_expected)
    result = value_unit(vm_get_frame(vm), vm->current_frame_index);

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
  DA_APPEND(vm.frames, (StackFrame) {0});

  ListNode *args = arena_alloc(&vm_get_frame(&vm)->arena, sizeof(ListNode));
  ListNode *args_end = args;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = arena_alloc(&vm_get_frame(&vm)->arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = arena_alloc(&vm_get_frame(&vm)->arena, sizeof(ListNode));
    new_arg->value = arena_alloc(&vm_get_frame(&vm)->arena, sizeof(Value));
    *new_arg->value = (Value) {
      ValueKindString,
      { .string = { buffer, len } },
      0,
    };
    new_arg->is_static = true;

    args_end->next = new_arg;
    args_end = new_arg;
  }

  vm_init(&vm, args, intrinsics);

  return vm;
}

void vm_init(Vm *vm, ListNode *args, Intrinsics *intrinsics) {
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

  vm->intrinsics = *intrinsics;
  vm->args = args;

  Value *unit_value = value_unit(vm_get_frame(vm), 0);
  Var unit_var = { STR_LIT("unit"), unit_value, VarKindGlobal };
  DA_APPEND(vm->global_vars, unit_var);
}

void vm_destroy(Vm *vm) {
  if (vm->global_vars.items)
    free(vm->global_vars.items);
  if (vm->intrinsics.items)
    free(vm->intrinsics.items);

  for (u32 i = 0; i < vm->frames.len; ++i) {
    StackFrame *frame = vm->frames.items + i;

    for (u32 j = 0; j < frame->values.len; ++j)
      value_free(frame->values.items[j]);
    free(frame->values.items);
    arena_free(&frame->arena);
    free(frame->vars.items);
  }
  free(vm->frames.items);
}

void begin_frame(Vm *vm) {
  if (++vm->current_frame_index == vm->frames.len)
    DA_APPEND(vm->frames, (StackFrame) {0});
}

void end_frame(Vm *vm) {
  StackFrame *frame = vm_get_frame(vm);

  for (u32 i = 0; i < frame->values.len; ++i)
    value_free(frame->values.items[i]);
  frame->values.len = 0;
  arena_reset(&frame->arena);
  frame->vars.len = 0;
  --vm->current_frame_index;
}

StackFrame *vm_get_frame(Vm *vm) {
  return vm->frames.items + vm->current_frame_index;
}
