#include "aether/vm.h"
#include "aether/misc.h"
#include "intrinsics.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define STACK_DUMP_LEN      10
#define MAX_RECURSION_LEVEL 1000
#define MAX_TRACE_LEVEL     25

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG(*(meta).file_path), (meta).row + 1, (meta).col + 1

#define PANIC(meta, text)                   \
  do {                                      \
    PERROR(META_FMT, text, META_ARG(meta)); \
    vm->state = ExecStateExit;              \
    vm->exit_code = 1;                      \
    return;                                 \
  } while (false)

#define PANIC_ARGS(meta, text, ...)                      \
  do {                                                   \
    PERROR(META_FMT, text, META_ARG(meta), __VA_ARGS__); \
    vm->state = ExecStateExit;                           \
    vm->exit_code = 1;                                   \
    return;                                              \
  } while (false)

static void print_stack_dump(Values *stack, Arena *arena) {
  u32 begin = 0;

  if (stack->len >= STACK_DUMP_LEN)
    begin = stack->len - STACK_DUMP_LEN;

  for (u32 j = begin; j < stack->len; ++j) {
    fputs(" -> ", stdout);
    Str value = value_to_str(stack->items[j], false, true, arena);
    str_print(value);
    fputc('\n', stdout);
  }
}

static bool ensure_stack_len_is_enough(Vm *vm, u32 min_len, InstrMeta *meta) {
  if (vm->stack.len < min_len + vm->frame_begin) {
    INFO("Stack dump:\n");
    print_stack_dump(&vm->stack, &vm->current_frame->arena);
    ERROR(META_FMT"Not enough values on the stack: expected %u, got %u\n",
          META_ARG(*meta), min_len, vm->stack.len);
    vm->exit_code = 1;
    return false;
  }

  return true;
}

static Var *get_var_from(Vars *vars, Str name) {
  for (u32 i = vars->len; i > 0; --i) {
    Var *var = vars->items + i - 1;

    if (str_eq(var->name, name))
      return var;
  }

  return NULL;
}

static Var *get_var(Vm *vm, Str name) {
  Var *var = get_var_from(&vm->current_frame->vars, name);
  if (var)
    return var;

  if (vm->current_func) {
    var = get_var_from(&vm->current_func->catched_vars, name);
    if (var)
      return var;
  }

  var = get_var_from(&vm->frames->vars, name);
  if (var)
    return var;

  return NULL;
}

static bool value_list_matches_kinds(u32 args_len, Value **args,
                                     ValueKind *arg_kinds) {
  for (u32 i = 0; i < args_len; ++i)
    if (args[i]->kind != arg_kinds[i] && arg_kinds[i] != ValueKindUnit)
      return false;

  return true;
}

static Intrinsic *get_intrinsic(Vm *vm, Str name, u32 args_len, Value **args) {
  u64 index = str_hash(name) % INTRINSICS_HASH_TABLE_CAP;

  Intrinsic *entry = vm->intrinsics.items[index];
  while (entry) {
    if (str_eq(entry->name, name) &&
        entry->args_count == args_len &&
        value_list_matches_kinds(args_len, args, entry->arg_kinds))
      return entry;

    entry = entry->next;
  }

  return NULL;
}

static u32 get_instr_index(Vm *vm, u16 label_id) {
  LabelsTable *labels = vm->current_func_labels;

  u64 index = label_id % LABELS_HASH_TABLE_CAP;

  Label *entry = labels->items[index];
  while (entry) {
    if (entry->name_id == label_id)
      return entry->instr_index;

    entry = entry->next;
  }

  return (u32) -1;
}

void catch_vars(Vars *catched, Vm *vm, Instrs *instrs) {
  Da(Str) defined_vars_names = {0};

  for (u32 i = 0; i < instrs->len; ++i) {
    Instr *instr = instrs->items + i;

    if (instr->kind == InstrKindDefVar) {
      Str new_var_name = get_str(instr->as.def_var.name_id);;
      DA_ARENA_APPEND(defined_vars_names, new_var_name, &vm->current_frame->arena);
    } else if (instr->kind == InstrKindGetVar) {
      Str name = get_str(instr->as.get_var.name_id);
      bool found = false;

      for (u32 j = 0; j < defined_vars_names.len; ++j) {
        if (str_eq(defined_vars_names.items[j], name)) {
          found = true;

          break;
        }
      }

      if (!found) {
        Str name = get_str(instr->as.get_var.name_id);
        Var *var = NULL;

        if (vm->current_frame != vm->frames)
          var = get_var_from(&vm->current_frame->vars, name);

        if (!var && vm->current_func)
          var = get_var_from(&vm->current_func->catched_vars, name);

        if (var) {
          Var new_var = { name, var->value, };
          DA_ARENA_APPEND(*catched, new_var, &vm->current_frame->arena);
          DA_ARENA_APPEND(defined_vars_names, new_var.name, &vm->current_frame->arena);
        }
      }
    } else if (instr->kind == InstrKindFunc) {
      catch_vars(catched, vm, &instr->as.func.body->instrs);
    }
  }
}

void load_labels(Func *func) {
  if (func->loaded)
    return;

  for (u32 j = 0; j < func->instrs.len; ++j) {
    Instr *instr = func->instrs.items + j;
    if (instr->kind != InstrKindLabel)
      continue;

    u64 index = instr->as.label.name_id % LABELS_HASH_TABLE_CAP;

    Label *entry = func->labels.items[index];
    while (entry && entry->next)
      entry = entry->next;

    Label *label = malloc(sizeof(Label));
    *label = (Label) {
      instr->as.label.name_id,
      j, NULL,
    };

    if (entry)
      entry->next = label;
    else
      func->labels.items[index] = label;
  }

  func->loaded = true;
}

void execute_func(Vm *vm, FuncValue *func, InstrMeta *meta) {
  Value **args = vm->stack.items + vm->stack.len - func->args.len;

  if (func->intrinsic_name_id != (u16) -1) {
    Str intrinsic_name = get_str(func->intrinsic_name_id);
    Intrinsic *intrinsic = get_intrinsic(vm, intrinsic_name, func->args.len, args);
    if (!intrinsic) {
      if (meta)
        ERROR(META_FMT"Intrinsic ("STR_FMT,
              META_ARG(*meta), STR_ARG(intrinsic_name));
      else
        ERROR("Intrinsic ("STR_FMT,
              STR_ARG(intrinsic_name));

      for (u32 i = 0; i < func->args.len; ++i) {
        putc(' ', stderr);
        fprint_value(stderr, args[i], true, &vm->current_frame->arena);
      }

      fprintf(stderr, ") was not found\n");

      vm->state = ExecStateExit;
      vm->exit_code = 1;
      return;
    }

    Value *result = (intrinsic->func)(vm, args);

    if (!result)
      result = value_unit(vm->current_frame);

    DA_APPEND(vm->stack, result);

    return;
  }

  load_labels(func->body);

  begin_frame(vm);

  for (u32 i = 0; i < func->catched_vars.len; ++i) {
    Var *catched_var = func->catched_vars.items + i;

    if (!catched_var->value) {
      Var *var = get_var_from(&vm->frames->vars, catched_var->name);
      if (!var) {
        if (meta)
          ERROR(META_FMT"Symbol "STR_FMT" was not found\n",
                META_ARG(*meta), STR_ARG(catched_var->name));
        else
          ERROR("Symbol "STR_FMT" was not found\n",
                STR_ARG(catched_var->name));

        vm->state = ExecStateExit;
        vm->exit_code = 1;
        return;
      }

      ++var->value->refs_count;

      catched_var->value = var->value;
    }
  }

  for (u32 i = 0; i < func->args.len; ++i) {
    Str arg_name = get_str(func->args.items[i]);
    Value *arg_value = args[i];

    ++arg_value->refs_count;

    Var new_var = {
      arg_name,
      arg_value,
    };
    DA_APPEND(vm->current_frame->vars, new_var);
  }

  u32 prev_frame_begin = vm->frame_begin;
  FuncValue *prev_func = vm->current_func;
  LabelsTable *prev_labels = vm->current_func_labels;

  vm->frame_begin = vm->stack.len;
  vm->current_func = func;
  vm->current_func_labels = &func->body->labels;

  execute(vm, &func->body->instrs);

  if (vm->state == ExecStateExit && vm->exit_code != 0) {
    for (u32 j = vm->current_frame->calls_data.len; j > 0; --j) {
      if (vm->trace_level++ <= vm->max_trace_level)
        break;

      CallData *data = vm->current_frame->calls_data.items + j - 1;
      INFO("Trace: "STR_FMT":%u:%u "STR_FMT"\n",
           STR_ARG(*data->meta.file_path),
           data->meta.row + 1, data->meta.col + 1,
           STR_ARG(data->func_name));
    }
  }

  Value *result;
  if (vm->frame_begin == vm->stack.len)
    result = value_unit(vm->current_frame->prev);
  else
    result = value_clone(stack_last(vm), vm->current_frame->prev);

  vm->current_func_labels = prev_labels;
  vm->current_func = prev_func;
  vm->stack.len = vm->frame_begin;
  vm->frame_begin = prev_frame_begin;

  end_frame(vm);

  DA_APPEND(vm->stack, result);
}

static void tail_call(Vm *vm, FuncValue *func) {
  Value **args = vm->stack.items + vm->stack.len - func->args.len;

  for (u32 i = 0; i < func->args.len; ++i)
    args[i] = value_clone(args[i], vm->temp_frame);

  end_frame(vm);
  begin_frame(vm);

  vm->stack.len = vm->frame_begin;

  for (u32 i = 0; i < func->args.len; ++i) {
    Str arg_name = get_str(func->args.items[i]);
    Value *arg_value = value_clone(args[i], vm->current_frame);

    ++arg_value->refs_count;

    Var new_var = {
      arg_name,
      arg_value,
    };
    DA_APPEND(vm->current_frame->vars, new_var);
  }

  reset_frame(vm->temp_frame);
}

void execute(Vm *vm, Instrs *instrs) {
  u32 i = 0;

  while (i < instrs->len) {
    Instr *instr = instrs->items + i;

    switch (instr->kind) {
    case InstrKindString: {
      Str string = get_str(instr->as.string.string_id);
      Value *new_value = value_string(string, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindInt: {
      Value *new_value = value_int(instr->as._int._int, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindFloat: {
      Value *new_value = value_float(instr->as._float._float, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindBytes: {
      Value *new_value = value_bytes(instr->as.bytes.bytes, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindFunc: {
      FuncValue *new_func = arena_alloc(&vm->current_frame->arena, sizeof(FuncValue));
      new_func->args = instr->as.func.args;
      new_func->body = instr->as.func.body;
      new_func->intrinsic_name_id = instr->as.func.intrinsic_name_id;
      new_func->catched_vars = (Vars) {0};

      if (instr->as.func.intrinsic_name_id == (u16) -1) {
        catch_vars(&new_func->catched_vars, vm, &new_func->body->instrs);
        if (vm->state != ExecStateContinue)
          return;
      }

      Value *new_value = value_func(new_func, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindFuncCall: {
      if (!ensure_stack_len_is_enough(vm,
                                      instr->as.func_call.args_len + 1,
                                      &instr->meta))
        return;

      if (++vm->recursion_level >= MAX_RECURSION_LEVEL) {
        ERROR(META_FMT "Infinite recursion detected\n",
              META_ARG(instr->meta));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return;
      }

      u32 deep = instr->as.func_call.args_len + 1;
      Value *func = vm->stack.items[vm->stack.len - deep];

      if (func->kind != ValueKindFunc) {
        Str kind = value_to_str(func, true, false, &vm->current_frame->arena);

        INFO("Stack dump:\n");
        print_stack_dump(&vm->stack, &vm->current_frame->arena);

        ERROR(META_FMT "Value of type "STR_FMT" is not callable\n",
              META_ARG(instr->meta), STR_ARG(kind));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        return;
      }

      if (func->as.func->args.len != instr->as.func_call.args_len)
        PANIC_ARGS(instr->meta, "Expected %u arguments, got %u\n",
                   func->as.func->args.len, instr->as.func_call.args_len);

      if (func->as.func == vm->current_func &&
          (i + 1 == instrs->len ||
           instrs->items[i + 1].kind == InstrKindRet)) {
        tail_call(vm, func->as.func);

        Str func_name = STR_LIT("<lambda>");
        u32 name_index = i - instr->as.func_call.args_instrs_len - 1;
        if (instrs->items[name_index].kind == InstrKindGetVar) {
          u16 name_id = instrs->items[name_index].as.get_var.name_id;
          func_name = get_str(name_id);
        }

        CallData data = {
          func_name,
          instr->meta,
        };
        DA_APPEND(vm->current_frame->calls_data, data);

        if (vm->state == ExecStateReturn)
          vm->state = ExecStateContinue;

        i = (u32) -1;
        break;
      }

      execute_func(vm, func->as.func, &instr->meta);
      if (vm->state == ExecStateExit) {
        if (vm->exit_code != 0) {
          for (u32 j = vm->current_frame->calls_data.len; j > 0; --j) {
            if (vm->trace_level++ < vm->max_trace_level)
              break;

            CallData *data = vm->current_frame->calls_data.items + j - 1;
            INFO("Trace: "STR_FMT":%u:%u "STR_FMT"\n",
                 STR_ARG(*data->meta.file_path),
                 data->meta.row + 1, data->meta.col + 1,
                 STR_ARG(data->func_name));
          }

          if (vm->trace_level++ < vm->max_trace_level) {
            Str func_name = STR_LIT("<lambda>");
            u32 name_index = i - instr->as.func_call.args_instrs_len - 1;
            if (instrs->items[name_index].kind == InstrKindGetVar) {
              u16 name_id = instrs->items[name_index].as.get_var.name_id;
              func_name = get_str(name_id);
            }

            INFO("Trace: "STR_FMT":%u:%u "STR_FMT"\n",
                 STR_ARG(*instr->meta.file_path),
                 instr->meta.row + 1, instr->meta.col + 1,
                 STR_ARG(func_name));
          }
        }

        return;
      }

      if (vm->state == ExecStateReturn)
        vm->state = ExecStateContinue;

      Value *result = stack_last(vm);
      vm->stack.len -= instr->as.func_call.args_len + 1;
      vm->stack.items[vm->stack.len - 1] = result;

      --vm->recursion_level;
    } break;

    case InstrKindDefVar: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *last = stack_last(vm);
      ++last->refs_count;
      Str name = get_str(instr->as.def_var.name_id);
      Var new_var = { name, last };
      DA_APPEND(vm->current_frame->vars, new_var);

      --vm->stack.len;
    } break;

    case InstrKindGetVar: {
      Str name = get_str(instr->as.get_var.name_id);
      Var *var = get_var(vm, name);
      if (!var)
        PANIC_ARGS(instr->meta, "Symbol "STR_FMT" was not found\n",
                   STR_ARG(name));

      DA_APPEND(vm->stack, var->value);
    } break;

    case InstrKindGet: {
      if (!ensure_stack_len_is_enough(vm, instr->as.get.chain_len, &instr->meta))
        return;

      Value *value = vm->stack.items[vm->stack.len - instr->as.get.chain_len];

      for (u32 j = 1; j < instr->as.get.chain_len; ++j) {
        Value *key = vm->stack.items[vm->stack.len - instr->as.get.chain_len + j];

        if (value->kind == ValueKindString) {
          if (key->kind != ValueKindInt)
            PANIC(instr->meta, "Value of type string can only be indexed with integer\n");

          Value *sub_string = get_from_string(vm, value->as.string.str, key->as._int);

          if (!sub_string) {
            value = value_unit(vm->current_frame);
            break;
          }

          value = sub_string;
        } else if (value->kind == ValueKindBytes) {
          if (key->kind != ValueKindInt)
            PANIC(instr->meta, "Value of type bytes can only be indexed with integer\n");

          if (key->as._int >= value->as.bytes.len)
            PANIC(instr->meta, "String index out of bounds\n");

          value = value_int(value->as.bytes.ptr[key->as._int], vm->current_frame);
        } else {
          Value **root = get_child_root(value, key, &instr->meta, vm);
          if (vm->state != ExecStateContinue)
            return;

          if (!root) {
            value = value_unit(vm->current_frame);
            break;
          }

          value = *root;
        }
      }

      vm->stack.len -= instr->as.get.chain_len;

      DA_APPEND(vm->stack, value);
    } break;

    case InstrKindSet: {
      if (!ensure_stack_len_is_enough(vm, instr->as.set.chain_len + 1, &instr->meta))
        return;

      Str name = get_str(instr->as.set.name_id);
      Var *var = get_var(vm, name);
      if (!var)
        PANIC_ARGS(instr->meta, "Symbol "STR_FMT" was not found\n",
                   STR_ARG(name));


      if (var->value->refs_count > 1 && !var->value->is_atom)
        var->value = value_clone(var->value, var->value->frame);

      Value *parent = var->value;
      Value *new_value = vm->stack.items[vm->stack.len - 1];
      Value **root = &var->value;

      for (u32 j = 0; j < instr->as.set.chain_len; ++j) {
        Value *key = vm->stack.items[vm->stack.len - instr->as.set.chain_len + j - 1];

        if (parent->kind == ValueKindString) {
          PANIC(instr->meta, "Value of type string cannot be mutated\n");
        } else if (parent->kind == ValueKindBytes) {
          PANIC(instr->meta, "Value of type bytes cannot be mutated\n");
        } else {
          root = get_child_root(parent, key, &instr->meta, vm);
          if (vm->state != ExecStateContinue)
            return;

          if (!root && parent->kind == ValueKindDict) {
            if (key->frame == parent->frame)
              ++key->refs_count;
            else
              key = value_clone(key, parent->frame);

            dict_set_value(parent->frame, parent->as.dict, key,
                           value_unit(parent->frame));
            root = get_child_root(parent, key, &instr->meta, vm);
          }

          if (!root) {
            ERROR(META_FMT"Value of type ", META_ARG(instr->meta));
            fprint_value(stderr, parent, true, &vm->current_frame->arena);
            fputs(" cannot be indexed with ", stderr);
            fprint_value(stderr, key, true, &vm->current_frame->arena);
            fputc('\n', stderr);
            vm->state = ExecStateExit;
            vm->exit_code = 1;
            return;
          }

          parent = *root;
        }
      }

      if (new_value->frame == parent->frame)
        ++new_value->refs_count;
      else
        new_value = value_clone(new_value, parent->frame);

      vm->stack.len -= instr->as.set.chain_len + 1;

      --(*root)->refs_count;
      *root = new_value;
    } break;

    case InstrKindJump: {
      u32 target = get_instr_index(vm, instr->as.jump.label_id);
      if (target == (u32) -1) {
        Str label = get_str(instr->as.jump.label_id);
        PANIC_ARGS(instr->meta, "Target label was not found: "STR_FMT"\n",
                   STR_ARG(label));
      }

      i = target - 1;
    } break;

    case InstrKindCondJump: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *cond = stack_last(vm);

      if (value_to_bool(cond)) {
        u32 target = get_instr_index(vm, instr->as.cond_jump.label_id);
        if (target == (u32) -1) {
          Str label = get_str(instr->as.cond_jump.label_id);
          PANIC_ARGS(instr->meta, "Target label was not found: "STR_FMT"\n",
                     STR_ARG(label));
        }

        i = target - 1;
      }

      --vm->stack.len;
    } break;

    case InstrKindCondNotJump: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *cond = stack_last(vm);

      if (!value_to_bool(cond)) {
        u32 target = get_instr_index(vm, instr->as.cond_not_jump.label_id);
        if (target == (u32) -1) {
          Str label = get_str(instr->as.cond_not_jump.label_id);
          PANIC_ARGS(instr->meta, "Target label was not found: "STR_FMT"\n",
                     STR_ARG(label));
        }

        i = target - 1;
      }

      --vm->stack.len;
    } break;

    case InstrKindLabel: break;

    case InstrKindMatchBegin: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      DA_APPEND(vm->current_frame->match_values, stack_last(vm));

      --vm->stack.len;
    } break;

    case InstrKindMatchCase: {
      if (vm->current_frame->match_values.len == 0)
        PANIC(instr->meta, "Match case not inside of a match block\n");

      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Values *match_stack = &vm->current_frame->match_values;
      Value *match_cond = match_stack->items[match_stack->len - 1];
      Value *case_cond = stack_last(vm);

      if (!value_eq(case_cond, match_cond)) {
        u32 target = get_instr_index(vm, instr->as.match_case.not_label_id);
        if (target == (u32) -1) {
          Str not_label = get_str(instr->as.match_case.not_label_id);
          PANIC_ARGS(instr->meta, "Target label not found: "STR_FMT"\n",
                     STR_ARG(not_label));
        }

        i = target - 1;
      }

      --vm->stack.len;
    } break;

    case InstrKindMatchEnd: {
      if (vm->current_frame->match_values.len == 0)
        PANIC(instr->meta, "Match end not inside of a match block\n");

      --vm->current_frame->match_values.len;
    } break;

    case InstrKindRet: {
      vm->state = ExecStateReturn;
      return;
    } break;

    case InstrKindList: {
      if (!ensure_stack_len_is_enough(vm, instr->as.list.len, &instr->meta))
        return;

      ListNode *new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      ListNode *node = new_list;

      for (u32 j = 0; j < instr->as.list.len; ++j) {
        node->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
        node->next->value = vm->stack.items[vm->stack.len - instr->as.list.len + j];

        ++node->next->value->refs_count;

        node = node->next;
      }

      node->next = NULL;

      vm->stack.len -= instr->as.list.len;

      Value *new_value = value_list(new_list, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindDict: {
      if (!ensure_stack_len_is_enough(vm, instr->as.dict.len * 2, &instr->meta))
        return;

      Dict *new_dict = arena_alloc(&vm->current_frame->arena, sizeof(Dict));
      memset(new_dict, 0, sizeof(Dict));

      for (u32 j = 0; j < instr->as.dict.len; ++j) {
        Value *key = vm->stack.items[vm->stack.len - instr->as.list.len * 2 + j * 2];
        Value *value = vm->stack.items[vm->stack.len - instr->as.list.len * 2 + j * 2 + 1];

        ++key->refs_count;
        ++value->refs_count;

        dict_set_value(vm->current_frame, new_dict, key, value);
      }

      vm->stack.len -= instr->as.dict.len * 2;

      Value *new_value = value_dict(new_dict, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindSelf: {
      Value *current_func = value_func(vm->current_func,
                                       vm->current_frame);
      DA_APPEND(vm->stack, current_func);
    } break;
    }

    ++i;
  }
}

Value *execute_get(Vm *vm, Func *func) {
  vm->current_func_labels = &func->labels;

  load_labels(func);
  execute(vm, &func->instrs);

  if (vm->stack.len == 0)
    return value_unit(vm->frames);
  else
    return vm->stack.items[--vm->stack.len];
}

Value *stack_last(Vm *vm) {
  return vm->stack.items[vm->stack.len - 1];
}

void intrinsics_append(Intrinsics *a, Intrinsic *b, u32 b_len, Arena *arena) {
  for (u32 i = 0; i < b_len; ++i) {
    u64 index = str_hash(b[i].name) % INTRINSICS_HASH_TABLE_CAP;

    Intrinsic *entry = a->items[index];
    while (entry && entry->next)
      entry = entry->next;

    Intrinsic *intrinsic = arena_alloc(arena, sizeof(Intrinsic));
    *intrinsic = b[i];

    if (entry)
      entry->next = intrinsic;
    else
      a->items[index] = intrinsic;
  }
}

Vm vm_create(i32 argc, char **argv, Intrinsics *intrinsics) {
  Vm vm = {0};

  vm.frames = malloc(sizeof(StackFrame));
  *vm.frames = (StackFrame) {0};
  vm.frames_end = vm.frames;

  vm.current_frame = vm.frames;

  vm.temp_frame = malloc(sizeof(StackFrame));
  *vm.temp_frame = (StackFrame) {0};

  vm.max_trace_level = MAX_TRACE_LEVEL;

  ListNode *args = arena_alloc(&vm.current_frame->arena, sizeof(ListNode));
  ListNode *args_end = args;
  for (u32 i = 0; i < (u32) argc; ++i) {
    u32 len = strlen(argv[i]);
    char *buffer = arena_alloc(&vm.current_frame->arena, len);
    memcpy(buffer, argv[i], len);

    ListNode *new_arg = arena_alloc(&vm.current_frame->arena, sizeof(ListNode));
    new_arg->value = value_string(STR(buffer, len), vm.current_frame);

    args_end->next = new_arg;
    args_end = new_arg;
  }

  args_end->next = NULL;

  vm_init(&vm, args, intrinsics);

  return vm;
}

void vm_init(Vm *vm, ListNode *args, Intrinsics *intrinsics) {
  intrinsics_append(intrinsics, core_intrinsics, core_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, math_intrinsics, math_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, str_intrinsics, str_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, random_intrinsics, random_intrinsics_len, &vm->frames->arena);
#ifndef NOSYSTEM
  intrinsics_append(intrinsics, base_intrinsics, base_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, io_intrinsics, io_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, path_intrinsics, path_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, net_intrinsics, net_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, term_intrinsics, term_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, system_intrinsics, system_intrinsics_len, &vm->frames->arena);
#endif
#ifdef GLASS
  intrinsics_append(intrinsics, glass_intrinsics, glass_intrinsics_len, &vm->frames->arena);
#endif
#ifdef EMSCRIPTEN
  intrinsics_append(intrinsics, web_intrinsics, web_intrinsics_len, &vm->frames->arena);
#endif
#ifdef CRYPTO
  intrinsics_append(intrinsics, crypto_intrinsics, crypto_intrinsics_len, &vm->frames->arena);
#endif

  vm->intrinsics = *intrinsics;
  vm->args = args;

  Value *unit_value = value_unit(vm->current_frame);
  Var unit_var = { STR_LIT("unit"), unit_value };
  DA_APPEND(vm->frames->vars, unit_var);

#ifdef EMSCRIPTEN
  Value *platform_value = value_string(STR_LIT("web"), vm->current_frame);
#else
  Value *platform_value = value_string(STR_LIT("linux"), vm->current_frame);
#endif

  Var platform_var = { STR_LIT("current-platform"), platform_value };
  DA_APPEND(vm->frames->vars, platform_var);

  Value *true_value = value_bool(true, vm->current_frame);
  Var true_var = { STR_LIT("true"), true_value };
  DA_APPEND(vm->frames->vars, true_var);

  Value *false_value = value_bool(false, vm->current_frame);
  Var false_var = { STR_LIT("false"), false_value };
  DA_APPEND(vm->frames->vars, false_var);
}

void vm_stop(Vm *vm) {
  vm->state = ExecStateExit;

  StackFrame *frame = vm->frames;
  while (frame) {
    for (u32 i = 0; i < frame->values.len; ++i)
      if (frame->values.items[i]->kind == ValueKindEnv)
        vm_stop(frame->values.items[i]->as.env->vm);

    frame = frame->next;
  }
}

void vm_destroy(Vm *vm) {
#ifdef EMSCRIPTEN
  while (vm->pending_fetches > 0);
#endif

  StackFrame *frame = vm->frames;
  while (frame) {
    StackFrame *next = frame->next;
    frame_free(frame);
    frame = next;
  }

  frame_free(vm->temp_frame);

  if (vm->stack.items)
    free(vm->stack.items);
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

void reset_frame(StackFrame *frame) {
  for (u32 i = 0; i < frame->values.len; ++i)
    value_free(frame->values.items[i]);
  frame->values.len = 0;

  arena_reset(&frame->arena);

  for (u32 i = 0; i < frame->vars.len; ++i)
    --frame->vars.items[i].value->refs_count;
  frame->vars.len = 0;

  for (u32 i = 0; i < frame->match_values.len; ++i)
    value_free(frame->match_values.items[i]);
  frame->match_values.len = 0;
}

void end_frame(Vm *vm) {
  reset_frame(vm->current_frame);
  if (vm->current_frame->prev)
    vm->current_frame = vm->current_frame->prev;
}
