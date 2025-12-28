#include "aether/vm.h"
#include "aether/misc.h"
#include "intrinsics.h"
#include "lexgen/runtime.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define WHILE_FRAME_LENGTH 100

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG(*(meta).file_path), (meta).row + 1, (meta).col + 1

#define CATCH_VARS(vm, local_names, catched_values, frame, expr) \
  do {                                                           \
    catch_vars(vm, local_names, catched_values, frame, expr);    \
    if (vm->state == ExecStateExit ||                            \
        vm->state == ExecStateReturn)                            \
      return;                                                    \
  } while (0)

#define CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, block) \
  do {                                                                  \
    catch_vars_block(vm, local_names, catched_values, frame, block);    \
    if (vm->state == ExecStateExit ||                                   \
        vm->state == ExecStateReturn)                                   \
      return;                                                           \
  } while (0)

typedef Da(Str) Strs;

static Value unit = { ValueKindUnit, {}, NULL, 0, false };

ListNode *list_clone(ListNode *nodes, StackFrame *frame) {
  if (!nodes)
    return NULL;

  ListNode *new_list = NULL;
  ListNode **new_list_next = &new_list;
  ListNode *node = nodes;
  while (node) {
    *new_list_next = arena_alloc(&frame->arena, sizeof(ListNode));
    (*new_list_next)->value = value_clone(node->value, frame);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  return new_list;
}

Dict dict_clone(Dict *dict, StackFrame *frame) {
  Dict copy = {
    arena_alloc(&frame->arena, dict->len * sizeof(DictValue)),
    dict->len,
    dict->len,
  };

  for (u32 i = 0; i < copy.len; ++i) {
    copy.items[i].key = value_clone(dict->items[i].key, frame);
    copy.items[i].value = value_clone(dict->items[i].value, frame);
  }

  return copy;
}

Value *value_unit(StackFrame *frame) {
  (void) frame;

  return &unit;
}

Value *value_list(ListNode *nodes, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindList, { .list = nodes },
                     frame, 0, false };
  return value;
}

Value *value_string(Str string, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindString, { .string = { string } },
                     frame, 0, false };
  return value;
}

Value *value_bytes(Bytes bytes, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindBytes, { .bytes = bytes },
                     frame, 0, false };
  return value;
}

Value *value_int(i64 _int, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindInt, { ._int = _int },
                     frame, 0, false };
  return value;
}

Value *value_float(f64 _float, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindFloat, { ._float = _float },
                     frame, 0, false };
  return value;
}

Value *value_bool(bool _bool, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindBool, { ._bool = _bool },
                     frame, 0, false };
  return value;
}

Value *value_dict(Dict dict, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindDict, { .dict = dict },
                     frame, 0, false };
  return value;
}

Value *value_func(Func func, StackFrame *frame) {
  Value *value = value_alloc(frame);
  Func *_func = arena_alloc(&frame->arena, sizeof(Func));
  *_func = func;
  _func->refs_count = 1;

  *value = (Value) { ValueKindFunc, { .func = _func },
                     frame, 0, false };
  return value;
}

Value *value_env(Vm vm, StackFrame *frame) {
  Value *value = value_alloc(frame);
  Env *env = arena_alloc(&frame->arena, sizeof(Env));
  *env = (Env) {0};
  env->vm = vm;
  env->refs_count = 1;
  *value = (Value) { ValueKindEnv, { .env = env },
                     frame, 0, false };
  return value;
}

Value *value_alloc(StackFrame *frame) {
  Value *value = arena_alloc(&frame->arena, sizeof(Value));
  DA_APPEND(frame->values, value);

  return value;
}

Value *value_clone(Value *value, StackFrame *frame) {
  if (value->kind == ValueKindUnit || !frame)
    return value;

  Value *copy = value_alloc(frame);
  *copy = *value;
  copy->frame = frame;
  copy->refs_count = 1;

  if (value->kind == ValueKindList) {
    copy->as.list = arena_alloc(&frame->arena, sizeof(ListNode));
    copy->as.list->next = list_clone(value->as.list->next, frame);
  } else if (value->kind == ValueKindString) {
    copy->as.string.str.len = value->as.string.str.len;
    copy->as.string.str.ptr = arena_alloc(&frame->arena, copy->as.string.str.len);
    memcpy(copy->as.string.str.ptr, value->as.string.str.ptr, copy->as.string.str.len);
  } else if (value->kind == ValueKindBytes) {
    copy->as.bytes.len = value->as.bytes.len;
    copy->as.bytes.ptr = arena_alloc(&frame->arena, copy->as.bytes.len);
    memcpy(copy->as.bytes.ptr, value->as.bytes.ptr, copy->as.bytes.len);
  } else if (value->kind == ValueKindDict) {
    copy->as.dict = dict_clone(&value->as.dict, frame);
  } else if (value->kind == ValueKindFunc) {
    ++copy->as.func->refs_count;
    if (frame != value->frame) {
      copy->as.func = arena_alloc(&frame->arena, sizeof(Func));
      *copy->as.func = *value->as.func;

      copy->as.func->catched_values_names.items =
        arena_alloc(&frame->arena, copy->as.func->catched_values_names.len *
                                   sizeof(NamedValue));

      for (u32 i = 0; i < copy->as.func->catched_values_names.len; ++i) {
        copy->as.func->catched_values_names.items[i].name =
          value->as.func->catched_values_names.items[i].name;
        copy->as.func->catched_values_names.items[i].value =
          value_clone(value->as.func->catched_values_names.items[i].value, frame);
      }
    }
  } else if (value->kind == ValueKindEnv) {
    ++copy->as.env->refs_count;
    if (frame != value->frame) {
      copy->as.env = arena_alloc(&frame->arena, sizeof(Env));
      *copy->as.env = *value->as.env;
    }
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
    if (--value->as.func->refs_count == 0)
      value->as.func->catched_values_names.len = 0;
  } else if (value->kind == ValueKindEnv) {
    if (--value->as.env->refs_count == 0) {
      if (value->as.env->macros.items)
        free(value->as.env->macros.items);

      if (value->as.env->included_files.items)
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
    if (a->as.func->intrinsic_name.len > 0)
      return str_eq(a->as.func->intrinsic_name,
                    b->as.func->intrinsic_name);

    return false;
  }

  case ValueKindBytes: {
    if (a->as.bytes.len != b->as.bytes.len)
      return false;

    for (u32 i = 0; i < a->as.bytes.len; ++i)
      if (a->as.bytes.ptr[i] != b->as.bytes.ptr[i])
        return false;

    return true;
  } break;

  default: {
    return false;
  }
  }
}

static Var *get_var_in_frame(StackFrame *frame, Vars *global_vars, Str name) {
  while (frame) {
    for (u32 i = frame->vars.len; i > 0; --i)
      if (str_eq(frame->vars.items[i - 1].name, name))
        return frame->vars.items + i - 1;

    if (!frame->can_lookup_through)
      break;

    frame = frame->prev;
  }

  for (u32 i = global_vars->len; i > 0; --i)
    if (str_eq(global_vars->items[i - 1].name, name))
      return global_vars->items + i - 1;

  return NULL;
}

static Var *get_var(Vm *vm, Str name) {
  return get_var_in_frame(vm->current_frame, &vm->global_vars, name);
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

    if (local_names->cap == local_names->len) {
      if (local_names->cap == 0)
        local_names->cap = 1;
      else
        local_names->cap *= 2;
      Str *new_items = arena_alloc(&frame->arena, local_names->cap * sizeof(Str));
      memcpy(new_items, local_names->items, local_names->len * sizeof(Str));
      local_names->items = new_items;
    }

    local_names->items[local_names->len++] = expr->as.var_def.name;
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
    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.set_at.dest, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.set_at.dest);
    if (var && var->kind != VarKindGlobal) {
      NamedValue value = {
        var->name,
        var->value,
      };

      if (catched_values->cap == catched_values->len) {
        if (catched_values->cap == 0)
          catched_values->cap = 1;
        else
          catched_values->cap *= 2;
        NamedValue *new_items = arena_alloc(&frame->arena, catched_values->cap * sizeof(NamedValue));
        memcpy(new_items, catched_values->items, catched_values->len * sizeof(NamedValue));
        catched_values->items = new_items;
      }

      catched_values->items[catched_values->len++] = value;
    }

    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.set_at.key);
    CATCH_VARS(vm, local_names, catched_values, frame, expr->as.set_at.value);
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.ret.expr);
  } break;

  case IrExprKindList: {
    CATCH_VARS_BLOCK(vm, local_names, catched_values, frame, &expr->as.list);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.ident, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.ident);
    if (var && var->kind != VarKindGlobal) {
      NamedValue value = {
        var->name,
        var->value,
      };

      if (catched_values->cap == catched_values->len) {
        if (catched_values->cap == 0)
          catched_values->cap = 1;
        else
          catched_values->cap *= 2;
        NamedValue *new_items = arena_alloc(&frame->arena, catched_values->cap * sizeof(NamedValue));
        memcpy(new_items, catched_values->items, catched_values->len * sizeof(NamedValue));
        catched_values->items = new_items;
      }

      catched_values->items[catched_values->len++] = value;
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    if (local_names->cap < local_names->len + expr->as.lambda.args.len) {
      local_names->cap = local_names->len + expr->as.lambda.args.len;
      Str *new_items = arena_alloc(&frame->arena, local_names->cap * sizeof(Str));
      memcpy(new_items, local_names->items, local_names->len * sizeof(Str));
      local_names->items = new_items;
    }

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      local_names->items[local_names->len++] = expr->as.lambda.args.items[i];

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

    if (expr->as.match.any)
      CATCH_VARS(vm, local_names, catched_values, frame, expr->as.match.any);
  } break;

  case IrExprKindSelf: break;
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

Value *execute_func(Vm *vm, Value **args, Func *func,
                    IrExprMeta *meta, bool value_expected) {
  if (func->intrinsic_name.len > 0) {
    Intrinsic *intrinsic = get_intrinsic(vm, func->intrinsic_name, func->args.len, args);

    if (!intrinsic) {
      StringBuilder sb = {0};
      sb_push_str(&sb, func->intrinsic_name);
      sb_push(&sb, " [");
      for (u32 i = 0; i < func->args.len; ++i) {
        if (i > 0)
          sb_push_char(&sb, ' ');
        SB_PUSH_VALUE(&sb, args[i], 0, true, true, vm);
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

      return value_unit(vm->current_frame);
    }

    Value *result = (*intrinsic->func)(vm, args);

    if (vm->state == ExecStateReturn)
      vm->state = ExecStateContinue;

    return result;
  }

  begin_frame(vm);

  StackFrame *frame = vm->current_frame;

  if (frame->vars.cap < func->args.len + func-> catched_values_names.len) {
    frame->vars.cap = func->catched_values_names.len + func->args.len;
    if (frame->vars.items)
      frame->vars.items = realloc(frame->vars.items, frame->vars.cap * sizeof(Var));
    else
      frame->vars.items = malloc(frame->vars.cap * sizeof(Var));
  }

  frame->vars.len = func->args.len + func->catched_values_names.len;

  for (u32 i = 0; i < func->args.len; ++i) {
    Var var = {
     func->args.items[i],
      args[i],
      VarKindLocal,
    };

    frame->vars.items[i] = var;
  }

  if (vm->current_func == func->parent_func) {
    frame->can_lookup_through = true;
  } else {
    for (u32 i = 0; i < func->catched_values_names.len; ++i) {
      Var var = {
       func->catched_values_names.items[i].name,
       func->catched_values_names.items[i].value,
        VarKindCatched,
      };

      frame->vars.items[i + func->args.len] = var;
    }
  }

  Func *prev_func = vm->current_func;

  vm->current_func = func;

  Value *result = execute_block(vm, &func->body, value_expected);

  vm->current_func = prev_func;

  frame->can_lookup_through = false;

  if (vm->state == ExecStateReturn)
    vm->state = ExecStateContinue;

  Value *result_stable = NULL;
  if (vm->state != ExecStateExit) {
    if (value_expected)
      result_stable = value_clone(result, frame->prev);
    else
      result_stable = value_unit(frame->prev);
  }

  end_frame(vm);

  return result_stable;
}

Value *execute_expr(Vm *vm, IrExpr *expr, bool value_expected) {
  Value *result = NULL;

  switch (expr->kind) {
  case IrExprKindBlock: {
    EXECUTE_BLOCK_SET(vm, result, &expr->as.block, value_expected);
  } break;

  case IrExprKindFuncCall: {
    Value *func_value;
    EXECUTE_EXPR_SET(vm, func_value, expr->as.func_call.func, true);

    if (func_value->kind != ValueKindFunc) {
      StringBuilder sb = {0};
      sb_push_value(&sb, func_value, 0, true, true, vm);

      PERROR(META_FMT, "Value of kind "STR_FMT" is not callable\n",
             META_ARG(expr->meta), STR_ARG(sb_to_str(sb)));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      free(sb.buffer);
      return value_unit(vm->current_frame);
    }

    if (expr->as.func_call.args.len != func_value->as.func->args.len) {
      PERROR(META_FMT, "Wrong arguments count: %u, expected %u\n",
            META_ARG(expr->meta), expr->as.func_call.args.len,
            func_value->as.func->args.len);
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm->current_frame);
    }

    Value **func_args = arena_alloc(&vm->current_frame->arena,
                                    expr->as.func_call.args.len * sizeof(Value *));

    for (u32 i = 0; i < expr->as.func_call.args.len; ++i) {
      func_args[i] = execute_expr(vm, expr->as.func_call.args.items[i], true);
      if (vm->state != ExecStateContinue)
        return value_unit(vm->current_frame);
    }

    result = execute_func(vm, func_args, func_value->as.func,
                          &expr->meta, value_expected);

    if (vm->state == ExecStateExit && vm->exit_code != 0) {
      Str name = STR_LIT("<lambda>");
      if (expr->as.func_call.func->kind == IrExprKindIdent)
        name = expr->as.func_call.func->as.ident;

      INFO("Trace: "STR_FMT":"STR_FMT":%u\n",
           STR_ARG(*expr->meta.file_path),
           STR_ARG(name), expr->meta.row + 1);

      return value_unit(vm->current_frame);
    }
  } break;

  case IrExprKindVarDef: {
    Value *value;
    EXECUTE_EXPR_SET(vm, value, expr->as.var_def.expr, true);

    if (value->frame == vm->current_frame)
      ++value->refs_count;
    else
      value = value_clone(value, vm->current_frame);

    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value = value;
    var.kind = vm->current_func != NULL ? VarKindLocal : VarKindGlobal;

    if (var.kind == VarKindGlobal)
      DA_APPEND(vm->global_vars, var);
    else
      DA_APPEND(vm->current_frame->vars, var);
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
    vm->current_frame->can_lookup_through = true;

    u32 i = 0;

    while (true) {
      Value *cond = execute_expr(vm, expr->as._while.cond, true);

      if (vm->state != ExecStateContinue) {
        if (vm->state == ExecStateBreak)
          vm->state = ExecStateContinue;

        break;
      }

      if (!value_to_bool(cond))
        break;

      Value *body_result = execute_block(vm, &expr->as._while.body, value_expected);

      if (vm->state != ExecStateContinue) {
        if (vm->state == ExecStateBreak)
          vm->state = ExecStateContinue;

        if (value_expected)
          result = value_clone(body_result, vm->current_frame->prev);

        break;
      }

      if (i++ == WHILE_FRAME_LENGTH) {
        end_frame(vm);
        begin_frame(vm);

        i = 0;
      }
    }

    vm->current_frame->can_lookup_through = false;
    end_frame(vm);
  } break;

  case IrExprKindSet: {
    Var *dest_var = get_var(vm, expr->as.set.dest);

    if (!dest_var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
        META_ARG(expr->meta), STR_ARG(expr->as.ident));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm->current_frame);
    }

    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.set.src, true);

    if (dest_var->value == src)
      break;

    --dest_var->value->refs_count;

    if (src->frame == dest_var->value->frame)
      ++src->refs_count;
    else
      src = value_clone(src, dest_var->value->frame);

    dest_var->value = src;
  } break;

  case IrExprKindGetAt: {
    if (!value_expected)
      break;

    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.get_at.src, true);
    Value *key;
    EXECUTE_EXPR_SET(vm, key, expr->as.get_at.key, true);

    if (src->kind == ValueKindList) {
      if (key->kind != ValueKindInt) {
        PERROR(META_FMT, "get: lists can only be indexed with integers\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      ListNode *node = src->as.list->next;
      u32 i = 0;
      while (node && i < (u32) key->as._int) {
        node = node->next;
        ++i;
      }

      if (node)
        result = node->value;
      else
        result = value_unit(vm->current_frame);
    } else if (src->kind == ValueKindString) {
      if (key->kind != ValueKindInt) {
        PERROR(META_FMT, "get: strings can only be indexed with integers\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      if ((u32) key->as._int >= src->as.string.str.len) {
        PERROR(META_FMT, "get: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      u32 index = 0;
      u32 byte_index = 0;
      u32 wchar_len;

      while (get_next_wchar(src->as.string.str, index, &wchar_len) != '\0') {
        if (index == key->as._int) {
          Str sub_string = {
            src->as.string.str.ptr + byte_index,
            wchar_len,
          };
          return value_string(sub_string, vm->current_frame);
        }

        ++index;
        byte_index += wchar_len;
      }

      PERROR(META_FMT, "get: index out of bounds\n",
             META_ARG(expr->meta));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm->current_frame);
    } else if (src->kind == ValueKindBytes) {
      if (key->as._int >= src->as.bytes.len) {
        PERROR(META_FMT, "get: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      result = value_int(src->as.bytes.ptr[key->as._int], vm->current_frame);
    }  else if (src->kind == ValueKindDict) {
      bool found = false;

      for (u32 i = 0; i < src->as.dict.len; ++i) {
        if (value_eq(src->as.dict.items[i].key, key)) {
          result = src->as.dict.items[i].value;
          found = true;

          break;
        }
      }

      if (!found)
        result = value_unit(vm->current_frame);
    } else {
      StringBuilder sb = {0};
      sb_push_value(&sb, src, 0, true, true, vm);

      PERROR(META_FMT, "get: source should be list, string or dictionary, but got "STR_FMT"\n",
             META_ARG(expr->meta), STR_ARG(sb_to_str(sb)));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      free(sb.buffer);

      return value_unit(vm->current_frame);
    }
  } break;

  case IrExprKindSetAt: {
    Var *dest_var = get_var(vm, expr->as.set_at.dest);
    if (!dest_var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
            META_ARG(expr->meta), STR_ARG(expr->as.set_at.dest));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm->current_frame);
    }

    if (dest_var->value->refs_count > 1 && !dest_var->value->is_atom)
      dest_var->value = value_clone(dest_var->value, dest_var->value->frame);

    Value *key;
    EXECUTE_EXPR_SET(vm, key, expr->as.set_at.key, true);
    Value *value;
    EXECUTE_EXPR_SET(vm, value, expr->as.set_at.value, true);

    if (value->frame == dest_var->value->frame)
      ++value->refs_count;
    else
      value = value_clone(value, dest_var->value->frame);

    if (dest_var->value->kind == ValueKindList) {
      if (key->kind != ValueKindInt) {
        PERROR(META_FMT, "set: only integer can be used as a list index\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
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

        return value_unit(vm->current_frame);
      }

      --node->value->refs_count;

      node->value = value;
    } else if (dest_var->value->kind == ValueKindBytes) {
      if (key->kind != ValueKindInt) {
        PERROR(META_FMT, "set: only integer can be used as a bytes index\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      if (value->kind != ValueKindInt) {
        PERROR(META_FMT, "set: only integer value can be assigned to a bytes array\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      if (key->as._int >= dest_var->value->as.bytes.len) {
        PERROR(META_FMT, "set: index out of bounds\n",
               META_ARG(expr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return value_unit(vm->current_frame);
      }

      dest_var->value->as.bytes.ptr[key->as._int] = value->as._int;
    } else if (dest_var->value->kind == ValueKindDict) {
      bool found = false;

      for (u32 i = 0; i < dest_var->value->as.dict.len; ++i) {
        if (value_eq(dest_var->value->as.dict.items[i].key, key)) {
          --dest_var->value->as.dict.items[i].value->refs_count;

          dest_var->value->as.dict.items[i].value = value;
          found = true;

          break;
        }
      }

      if (!found) {
        if (key->frame == dest_var->value->frame)
          ++key->refs_count;
        else
          key = value_clone(key, dest_var->value->frame);

        DictValue dict_value = { key, value };

        Dict *dict = &dest_var->value->as.dict;

        if (dict->cap == dict->len) {
          if (dict->cap == 0)
            dict->cap = 1;
          else
            dict->cap *= 2;

          DictValue *new_items =
            arena_alloc(&dest_var->value->frame->arena,
                        dict->cap * sizeof(DictValue));

          memcpy(new_items, dict->items, dict->len * sizeof(DictValue));

          dict->items = new_items;
        }

        dict->items[dict->len++] = dict_value;
      }
    } else {
      StringBuilder sb = {0};
      sb_push_value(&sb, dest_var->value, 0, true, true, vm);

      PERROR(META_FMT, "set: destination should be list or dictionary, but got "STR_FMT"\n",
             META_ARG(expr->meta), STR_ARG(sb_to_str(sb)));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      free(sb.buffer);

      return value_unit(vm->current_frame);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      EXECUTE_EXPR_SET(vm, result, expr->as.ret.expr, true);
    else
      result = value_unit(vm->current_frame);

    vm->state = ExecStateReturn;
  } break;

  case IrExprKindList: {
    if (!value_expected)
      break;

    ListNode *list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    ListNode *list_end = list;

    for (u32 i = 0; i < expr->as.list.len; ++i) {
      ListNode *new_node = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      EXECUTE_EXPR_SET(vm, new_node->value, expr->as.list.items[i], true);
      new_node->next = NULL;

      if (list_end) {
        list_end->next = new_node;
        list_end = new_node;
      } else {
        list = new_node;
        list_end = new_node;
      }
    }

    result = value_list(list, vm->current_frame);
  } break;

  case IrExprKindIdent: {
    if (!value_expected)
      break;

    Var *var = get_var(vm, expr->as.ident);
    if (!var) {
      PERROR(META_FMT, "Symbol "STR_FMT" was not defined before usage\n",
             META_ARG(expr->meta), STR_ARG(expr->as.ident));
      vm->state = ExecStateExit;
      vm->exit_code = 1;

      return value_unit(vm->current_frame);
    }

    result = var->value;
  } break;

  case IrExprKindString: {
    if (value_expected)
      result = value_string(expr->as.string, vm->current_frame);
  } break;

  case IrExprKindInt: {
    if (value_expected)
      result = value_int(expr->as._int, vm->current_frame);
  } break;

  case IrExprKindFloat: {
    if (value_expected)
      result = value_float(expr->as._float, vm->current_frame);
  } break;

  case IrExprKindBool: {
    if (value_expected)
      result = value_bool(expr->as._bool, vm->current_frame);
  } break;

   case IrExprKindLambda: {
    if (!value_expected)
      break;

    NamedValues catched_values_names = {0};

    Strs local_names;
    local_names.len = expr->as.lambda.args.len;
    local_names.cap = local_names.len;
    local_names.items = arena_alloc(&vm->current_frame->arena, local_names.len * sizeof(Str));
    memcpy(local_names.items, expr->as.lambda.args.items,
           expr->as.lambda.args.len * sizeof(Str));

    catch_vars_block(vm, &local_names, &catched_values_names,
                     vm->current_frame, &expr->as.lambda.body);

    Func func = {
      expr->as.lambda.args,
      expr->as.lambda.body,
      catched_values_names,
      vm->current_func,
      expr->as.lambda.intrinsic_name,
      1,
    };

    result = value_func(func, vm->current_frame);
  } break;

  case IrExprKindDict: {
    if (!value_expected)
      break;

    Dict dict = {0};
    dict.len = expr->as.dict.len;
    dict.cap = dict.len;
    dict.items = arena_alloc(&vm->current_frame->arena, dict.cap * sizeof(DictValue));

    for (u32 i = 0; i < dict.len; ++i) {
      DictValue field = {0};
      EXECUTE_EXPR_SET(vm, field.key, expr->as.dict.items[i].key, true);
      EXECUTE_EXPR_SET(vm, field.value, expr->as.dict.items[i].expr, true);

      dict.items[i] = field;
    }

    result = value_dict(dict, vm->current_frame);
  } break;

  case IrExprKindMatch: {
    Value *src;
    EXECUTE_EXPR_SET(vm, src, expr->as.match.src, true);

    bool found = false;

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      Value *pattern;
      EXECUTE_EXPR_SET(vm, pattern, expr->as.match.cases.items[i].pattern, true);

      if (value_eq(pattern, src)) {
        EXECUTE_EXPR_SET(vm, result, expr->as.match.cases.items[i].expr, value_expected);

        found = true;

        break;
      }
    }

    if (!found && expr->as.match.any)
      EXECUTE_EXPR_SET(vm, result, expr->as.match.any, true);
  } break;

  case IrExprKindSelf: {
    if (value_expected)
      result = value_func(*vm->current_func, vm->current_frame);
  } break;

  case IrExprKindBreak: {
    if (expr->as._break.expr)
      EXECUTE_EXPR_SET(vm, result, expr->as._break.expr, true);
    else
      result = value_unit(vm->current_frame);

    vm->state = ExecStateBreak;
  } break;
  }

  if (value_expected && !result)
    return value_unit(vm->current_frame);
  return result;
}

Value *execute_block(Vm *vm, IrBlock *block, bool value_expected) {
  Value *result = NULL;

  for (u32 i = 0; i + 1 < block->len; ++i)
    EXECUTE_EXPR_SET(vm, result, block->items[i], value_expected);

  if (block->len > 0)
    EXECUTE_EXPR_SET(vm, result, block->items[block->len - 1], value_expected);
  else if (value_expected)
    result = value_unit(vm->current_frame);

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

  vm.frames = malloc(sizeof(StackFrame));
  *vm.frames = (StackFrame) {0};
  vm.frames_end = vm.frames;

  vm.current_frame = vm.frames;

  ListNode *args = arena_alloc(&vm.current_frame->arena, sizeof(ListNode));
  ListNode *args_end = args;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = arena_alloc(&vm.current_frame->arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = arena_alloc(&vm.current_frame->arena, sizeof(ListNode));
    new_arg->value = arena_alloc(&vm.current_frame->arena, sizeof(Value));
    *new_arg->value = (Value) {
      ValueKindString,
      { .string = { { buffer, len } } },
      vm.current_frame,
      1, false,
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
#ifdef EMSCRIPTEN
  intrinsics_append(intrinsics, web_intrinsics, web_intrinsics_len);
#endif

  vm->intrinsics = *intrinsics;
  vm->args = args;

  Value *unit_value = value_unit(vm->current_frame);
  Var unit_var = { STR_LIT("unit"), unit_value, VarKindGlobal };
  DA_APPEND(vm->global_vars, unit_var);

#ifdef EMSCRIPTEN
  Value *platform_value = value_string(STR_LIT("web"), vm->current_frame);
#else
  Value *platform_value = value_string(STR_LIT("linux"), vm->current_frame);
#endif

  Var platform_var = { STR_LIT("current-platform"), platform_value, VarKindGlobal };
  DA_APPEND(vm->global_vars, platform_var);
}

void vm_stop(Vm *vm) {
  vm->state = ExecStateExit;

  StackFrame *frame = vm->frames;
  while (frame) {
    for (u32 i = 0; i < frame->values.len; ++i)
      if (frame->values.items[i]->kind == ValueKindEnv)
        vm_stop(&frame->values.items[i]->as.env->vm);

    frame = frame->next;
  }
}

static void frame_free(StackFrame *frame) {
  for (u32 i = 0; i < frame->values.len; ++i)
    value_free(frame->values.items[i]);
  if (frame->values.items)
    free(frame->values.items);
  frame->values.len = 0;
  arena_free(&frame->arena);
  if (frame->vars.items)
    free(frame->vars.items);
  frame->vars.len = 0;
  free(frame);
}

void vm_destroy(Vm *vm) {
  if (vm->global_vars.items)
    free(vm->global_vars.items);
  if (vm->intrinsics.items)
    free(vm->intrinsics.items);

  StackFrame *frame = vm->frames;
  while (frame) {
    StackFrame *next = frame->next;
    frame_free(frame);
    frame = next;
  }
}

void begin_frame(Vm *vm) {
  if (vm->current_frame->next == NULL) {
    vm->frames_end->next = malloc(sizeof(StackFrame));
    *vm->frames_end->next = (StackFrame) {0};
    vm->frames_end->next->prev = vm->frames_end;
    vm->frames_end = vm->frames_end->next;
    vm->current_frame->next = vm->frames_end;
  }

  vm->current_frame = vm->current_frame->next;
}

void end_frame(Vm *vm) {
  StackFrame *frame = vm->current_frame;

  for (u32 i = 0; i < frame->values.len; ++i)
    value_free(frame->values.items[i]);
  frame->values.len = 0;
  arena_reset(&frame->arena);
  frame->vars.len = 0;

  if (vm->current_frame->prev)
    vm->current_frame = vm->current_frame->prev;
}
