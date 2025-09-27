#include "aether-vm/vm.h"
#include "shl_str.h"
#include "shl_log.h"
#include "shl_arena.h"
#include "std-intrinsics.h"

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
  Value value = { ValueKindString, { .string = string } };
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

static void free_value(Value *value, RcArena *rc_arena) {
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list;
    while (node && !node->is_static) {
      ListNode *next_node = node->next;
      rc_arena_free(rc_arena, node);
      node = next_node;
    }
  } else if (value->kind == ValueKindString) {
    rc_arena_free(rc_arena, value->as.string.ptr);
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

static void catch_vars_block(Vm *vm, Strs *local_names, NamedValues *catched_values, IrBlock *block);

static void catch_vars(Vm *vm, Strs *local_names, NamedValues *catched_values, IrExpr *expr) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    catch_vars_block(vm, local_names, catched_values, &expr->as.block);
  } break;

  case IrExprKindFuncDef: break;

  case IrExprKindFuncCall: {
    catch_vars_block(vm, local_names, catched_values, &expr->as.func_call.args);
  } break;

  case IrExprKindVarDef: {
    catch_vars(vm, local_names, catched_values, expr->as.var_def.expr);

    DA_APPEND(*local_names, expr->as.var_def.name);
  } break;

  case IrExprKindIf: {
    catch_vars_block(vm, local_names, catched_values, &expr->as._if.if_body);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i)
      catch_vars_block(vm, local_names, catched_values, &expr->as._if.elifs.items[i].body);

    if (expr->as._if.has_else)
      catch_vars_block(vm, local_names, catched_values, &expr->as._if.else_body);
  } break;

  case IrExprKindWhile: {
    catch_vars_block(vm, local_names, catched_values, &expr->as._while.body);
  } break;

  case IrExprKindSet: {
    catch_vars(vm, local_names, catched_values, expr->as.set.src);
  } break;

  case IrExprKindField: {
    if (expr->as.field.is_set)
      catch_vars(vm, local_names, catched_values, expr->as.field.expr);
  } break;

  case IrExprKindList: {
    catch_vars_block(vm, local_names, catched_values, &expr->as.list.content);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < local_names->len; ++i)
      if (str_eq(expr->as.ident.ident, local_names->items[i]))
        return;

    Var *var = get_var(vm, expr->as.ident.ident);
    if (!var) {
      ERROR("Variable "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.ident.ident));
      exit(1);
    }

    NamedValue value = {
      var->name,
      vm->stack.items[var->value_index],
    };
    DA_APPEND(*catched_values, value);
  } break;

  case IrExprKindString: break;
  case IrExprKindNumber: break;
  case IrExprKindBool: break;
  case IrExprKindLambda: break;

  case IrExprKindRecord: {
    for (u32 i = 0; i < expr->as.record.len; ++i)
      catch_vars(vm, local_names, catched_values, expr->as.record.items[i].expr);
  } break;
  }
}

static void catch_vars_block(Vm *vm, Strs *local_names, NamedValues *catched_values, IrBlock *block) {
  for (u32 i = 0; i < block->len; ++i)
    catch_vars(vm, local_names, catched_values, block->items[i]);
}

static void execute_block(Vm *vm, IrBlock *block, bool value_expected);

static bool get_func(Vm *vm, Str name, u32 args_count, IrArgs *args,
                     IrBlock *body, NamedValues *catched_values) {
  Var *var = get_var(vm, name);
  if (var) {
    Value *value = vm->stack.items + var->value_index;
    if (value->kind == ValueKindFunc &&
        value->as.func.args.len == args_count) {
      *args = value->as.func.args;
      *body = value->as.func.body;
      *catched_values = value->as.func.catched_values;

      return true;
    }

    return false;
  }

  for (u32 i = vm->funcs.len; i > 0; --i) {
    Func *func = vm->funcs.items + i - 1;

    if (str_eq(func->def.name, name) &&
        func->def.args.len == args_count) {
      *args = func->def.args;
      *body = func->def.body;
      *catched_values = func->catched_values;

      return true;
    }
  }

  return false;
}

void execute_func(Vm *vm, Str name, ValueStack *args, bool value_expected) {
  IrArgs func_args = {0};
  IrBlock func_body = {0};
  NamedValues catched_values = {0};
  if (!get_func(vm, name, args->len, &func_args, &func_body, &catched_values)) {
    for (u32 i = vm->intrinsics.len; i > 0; --i) {
      Intrinsic *intrinsic = vm->intrinsics.items + i - 1;

      if (str_eq(intrinsic->name, name) &&
          (intrinsic->args_count == args->len ||
           intrinsic->args_count == (u32) -1)) {
        for (u32 j = 0; j < args->len; ++j)
          DA_APPEND(vm->stack, args->items[j]);

        intrinsic->func(vm);

        if (value_expected && !intrinsic->has_return_value)
          value_stack_push_unit(&vm->stack);
        else if (!value_expected && intrinsic->has_return_value)
          --vm->stack.len;

        return;
      }
    }

    ERROR("Function "STR_FMT" with %u arguments was not defined before usage\n",
          STR_ARG(name), args->len);
    exit(1);
  }

  u32 prev_stack_len = vm->stack.len;
  Vars prev_local_vars = vm->local_vars;
  bool prev_is_inside_of_func = vm->is_inside_of_func;

  vm->local_vars = (Vars) {0};
  vm->is_inside_of_func = true;

  for (u32 i = 0; i < func_args.len; ++i) {
    Var var = {
      func_args.items[i],
      vm->stack.len,
    };

    DA_APPEND(vm->stack, args->items[i]);
    DA_APPEND(vm->local_vars, var);
  }

  for (u32 i = 0; i < catched_values.len; ++i) {
    Var var = {
      catched_values.items[i].name,
      vm->stack.len,
    };

    DA_APPEND(vm->stack, catched_values.items[i].value);
    DA_APPEND(vm->local_vars, var);
  }

  execute_block(vm, &func_body, value_expected);

  for (u32 i = prev_stack_len; i < vm->stack.len - value_expected; ++i)
    free_value(vm->stack.items + i, vm->rc_arena);

  free(vm->local_vars.items);
  vm->local_vars = prev_local_vars;
  vm->is_inside_of_func = prev_is_inside_of_func;

  if (value_expected)
    vm->stack.items[prev_stack_len] = vm->stack.items[vm->stack.len - 1];
  vm->stack.len = prev_stack_len + value_expected;
}

void execute_expr(Vm *vm, IrExpr *expr, bool value_expected) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    execute_block(vm, &expr->as.block, value_expected);
  } break;

  case IrExprKindFuncDef: {
    for (u32 i = 0; i < vm->funcs.len; ++i) {
      if (str_eq(vm->funcs.items[i].def.name, expr->as.func_def.name) &&
          vm->funcs.items[i].def.args.len == expr->as.func_def.args.len) {
        ERROR("Function "STR_FMT" with %u args was redefined\n",
              STR_ARG(expr->as.func_def.name), expr->as.func_def.args.len);
        exit(1);
      }
    }

    Func func = { expr->as.func_def, {} };
    DA_APPEND(vm->funcs, func);

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindFuncCall: {
    ValueStack args = {0};

    for (u32 i = 0; i < expr->as.func_call.args.len; ++i) {
      execute_expr(vm, expr->as.func_call.args.items[i], true);
      Value arg = value_stack_pop(&vm->stack);
      DA_APPEND(args, arg);
    }

    execute_func(vm, expr->as.func_call.name, &args, value_expected);
  } break;

  case IrExprKindVarDef: {
    execute_expr(vm, expr->as.var_def.expr, true);

    Var *prev_var = get_var(vm, expr->as.var_def.name);
    if (prev_var) {
      Value *prev_value = vm->stack.items + prev_var->value_index;
      free_value(prev_value, vm->rc_arena);
      *prev_value = value_stack_pop(&vm->stack);

      if (value_expected)
        value_stack_push_unit(&vm->stack);

      break;
    }

    Var var = {0};
    var.name = expr->as.var_def.name;
    var.value_index = vm->stack.len - 1;

    if (vm->is_inside_of_func)
      DA_APPEND(vm->local_vars, var);
    else
      DA_APPEND(vm->global_vars, var);

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindIf: {
    execute_expr(vm, expr->as._if.cond, true);

    Value cond = value_stack_pop(&vm->stack);
    if (cond.kind != ValueKindBool) {
      ERROR("Only boolean value can be used as a condition\n");
      exit(1);
    }

    if (cond.as._bool) {
      execute_block(vm, &expr->as._if.if_body, value_expected);
      break;
    }

    bool executed_elif = false;

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      execute_expr(vm, expr->as._if.elifs.items[i].cond, true);

      cond = value_stack_pop(&vm->stack);
      if (cond.kind != ValueKindBool) {
        ERROR("Only boolean value can be used as a condition\n");
        exit(1);
      }

      if (cond.as._bool) {
        execute_block(vm, &expr->as._if.elifs.items[i].body, value_expected);
        executed_elif = true;
        break;
      }
    }

    if (expr->as._if.has_else && !executed_elif)
      execute_block(vm, &expr->as._if.else_body, value_expected);
    else if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindWhile: {
    bool is_first_iter = true;
    while (true) {
      if (!is_first_iter && value_expected)
        --vm->stack.len;

      execute_expr(vm, expr->as._while.cond, true);

      Value cond = value_stack_pop(&vm->stack);
      if (cond.kind != ValueKindBool) {
        ERROR("Only boolean value can be used as a condition\n");
        exit(1);
      }

      if (!cond.as._bool)
        break;

      is_first_iter = false;

      execute_block(vm, &expr->as._while.body, value_expected);
    }

    if (is_first_iter && value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindSet: {
    Var *var = get_var(vm, expr->as.set.dest);
    if (!var) {
      ERROR("Variable "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.set.dest));
      exit(1);
    }

    execute_expr(vm, expr->as.set.src, true);
    Value value = value_stack_pop(&vm->stack);
    vm->stack.items[var->value_index] = value;

    if (value_expected)
      value_stack_push_unit(&vm->stack);
  } break;

  case IrExprKindField: {
    Var *var = get_var(vm, expr->as.field.record);
    if (!var) {
      ERROR("Variable "STR_FMT" was not defined before usage\n",
            STR_ARG(expr->as.set.dest));
      exit(1);
    }

    Value *record_value = vm->stack.items + var->value_index;
    if (record_value->kind != ValueKindRecord) {
      ERROR("Only records have fields\n");
      exit(1);
    }

    NamedValue *field = NULL;

    for (u32 i = 0; i < record_value->as.record.len; ++i) {
      NamedValue *temp_field = record_value->as.record.items + i;

      if (str_eq(temp_field->name, expr->as.field.field)) {
        field = temp_field;
        break;
      }
    }

    if (!field) {
      ERROR("Field `"STR_FMT"` was not found in `"STR_FMT"`\n",
            STR_ARG(expr->as.field.field),
            STR_ARG(expr->as.field.record));
    }

    if (expr->as.field.is_set) {
      free_value(&field->value, vm->rc_arena);

      execute_expr(vm, expr->as.field.expr, true);
      field->value = value_stack_pop(&vm->stack);

      if (value_expected)
        value_stack_push_unit(&vm->stack);
    } else if (value_expected) {
      DA_APPEND(vm->stack, field->value);
    }
  } break;

  case IrExprKindList: {
    if (!value_expected)
      break;

    ListNode *list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    ListNode *list_end = list;

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

    Var *var = get_var(vm, expr->as.ident.ident);
    if (var) {
      DA_APPEND(vm->stack, vm->stack.items[var->value_index]);
      return;
    }

    for (u32 i = 0; i < vm->funcs.len; ++i) {
      Func *func = vm->funcs.items + i;

      if (str_eq(func->def.name, expr->as.ident.ident)) {
        Value func_value = {
          ValueKindFunc,
          { .func = { func->def.name, func->def.args, func->def.body, {} } },
        };
        DA_APPEND(vm->stack, func_value);
        return;
      }
    }

    ERROR("Variable "STR_FMT" was not defined before usage\n",
          STR_ARG(expr->as.ident.ident));
    exit(1);
  } break;

  case IrExprKindString: {
    if (value_expected)
      value_stack_push_string(&vm->stack, expr->as.string.lit);
  } break;

  case IrExprKindNumber: {
    if (value_expected)
      value_stack_push_number(&vm->stack, expr->as.number.number);
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

    catch_vars_block(vm, &local_names, &catched_values, &expr->as.lambda.body);

    Value func_value = {
      ValueKindFunc,
      {
        .func = {
          (Str) {0},
          expr->as.lambda.args,
          expr->as.lambda.body,
          catched_values,
        },
      },
    };
    DA_APPEND(vm->stack, func_value);

    Func func = {
      {
        (Str) {0},
        expr->as.lambda.args,
        expr->as.lambda.body,
      },
      catched_values,
    };
    DA_APPEND(vm->funcs, func);
  } break;

  case IrExprKindRecord: {
    if (!value_expected)
      break;

    Record record = {0};

    for (u32 i = 0; i < expr->as.record.len; ++i) {
      execute_expr(vm, expr->as.record.items[i].expr, true);

      NamedValue field;
      field.name = expr->as.record.items[i].name;
      field.value = value_stack_pop(&vm->stack);

      DA_APPEND(record, field);
    }

    value_stack_push_record(&vm->stack, record);
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

  vm.intrinsics.cap += std_intrinsics_len;
  vm.intrinsics.items = realloc(vm.intrinsics.items, vm.intrinsics.cap * sizeof(Intrinsic));
  memcpy(vm.intrinsics.items + vm.intrinsics.len, std_intrinsics,
         std_intrinsics_len * sizeof(Intrinsic));
  vm.intrinsics.len += std_intrinsics_len;

  vm.args = rc_arena_alloc(rc_arena, sizeof(ListNode));
  ListNode *args_end = vm.args;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = rc_arena_alloc(rc_arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = rc_arena_alloc(rc_arena, sizeof(ListNode));
    new_arg->value = (Value) {
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

  execute_block(&vm, ir, false);
}
