#include "aether/vm.h"
#include "aether/misc.h"
#include "intrinsics.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define STACK_DUMP_LEN 10

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG(*(meta).file_path), (meta).row + 1, (meta).col + 1

#define PANIC(meta, text)                 \
  do {                                    \
    if (match_stack.items)                \
      free(match_stack.items);            \
    ERROR(META_FMT text, META_ARG(meta)); \
    vm->state = ExecStateExit;            \
    vm->exit_code = 1;                    \
    return;                               \
  } while (false)

#define PANIC_ARGS(meta, text, ...)                    \
  do {                                                 \
    if (match_stack.items)                             \
      free(match_stack.items);                         \
    ERROR(META_FMT text, META_ARG(meta), __VA_ARGS__); \
    vm->state = ExecStateExit;                         \
    vm->exit_code = 1;                                 \
    return;                                            \
  } while (false)

static void print_stack_dump(Values *stack) {
  StringBuilder sb = {0};
  u32 begin = 0;

  if (stack->len >= STACK_DUMP_LEN)
    begin = stack->len - STACK_DUMP_LEN;

  for (u32 j = begin; j < stack->len; ++j) {
    sb_push(&sb, " -> ");
    sb_push_value(&sb, stack->items[j], 0, false, true);
    sb_push_char(&sb, '\n');
  }

  str_print(sb_to_str(sb));

  free(sb.buffer);
}

static bool ensure_stack_len_is_enough(Vm *vm, u32 min_len, InstrMeta *meta) {
  if (vm->stack.len < min_len) {
    INFO("Stack dump:\n");
    print_stack_dump(&vm->stack);
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

  if (vm->current_func)
  var = get_var_from(&vm->current_func->catched_vars, name);
  if (var)
    return var;

  var = get_var_from(&vm->frames->vars, name);
  if (var)
    return var;

  return NULL;
}

static bool value_list_matches_kinds(u32 args_len, Value **args,
                                     ValueKind *arg_kinds) {
  for (u32 i = 0; i < args_len; ++i)
    if (args[i]->kind != arg_kinds[i] && arg_kinds == ValueKindUnit)
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

static u32 get_instr_index(Vm *vm, Str label_name) {
  u32 func_index = 0;
  if (vm->current_func)
    func_index = vm->current_func->body_index;

  u64 index = str_hash(label_name) % LABELS_HASH_TABLE_CAP;

  Label *entry = vm->labels.items[func_index].items[index];
  while (entry) {
    if (str_eq(entry->name, label_name))
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
      Str new_var_name = instr->as.def_var.name;
      DA_ARENA_APPEND(defined_vars_names, new_var_name, &vm->current_frame->arena);
    } else if (instr->kind == InstrKindGetVar) {
      bool found = false;

      for (u32 j = 0; j < defined_vars_names.len; ++j) {
        if (str_eq(defined_vars_names.items[j], instr->as.get_var.name)) {
          found = true;

          break;
        }
      }

      if (!found) {
        Var *var = get_var_from(&vm->current_frame->vars, instr->as.get_var.name);
        if (var) {
          Var new_var = {
            instr->as.get_var.name,
            var->value,
          };
          DA_ARENA_APPEND(*catched, new_var, &vm->current_frame->arena);
          DA_ARENA_APPEND(defined_vars_names, new_var.name, &vm->current_frame->arena);
        }
      }
    }
  }
}

void execute_instrs(Vm *vm, Instrs *instrs);

void execute_func(Vm *vm, FuncValue *func, InstrMeta *meta, bool value_ignored) {
  Value **args = vm->stack.items + vm->stack.len - func->args.len;

  if (func->intrinsic_name.len > 0) {
    Intrinsic *intrinsic = get_intrinsic(vm, func->intrinsic_name, func->args.len, args);
    if (!intrinsic) {
      ERROR(META_FMT"Intrinsic "STR_FMT":%u was not found\n",
            META_ARG(*meta), STR_ARG(func->intrinsic_name), func->args.len);
      vm->state = ExecStateExit;
      vm->exit_code = 1;
      return;
    }

    Value *result = (intrinsic->func)(vm, args);

    if (!value_ignored) {
      if (!result)
        result = value_unit(vm->current_frame);

      DA_APPEND(vm->stack, result);
    }

    return;
  }

  begin_frame(vm);

  for (u32 i = 0; i < func->args.len; ++i) {
    Var new_var = {
      func->args.items[i],
      vm->stack.items[vm->stack.len - func->args.len + i],
    };
    DA_APPEND(vm->current_frame->vars, new_var);
  }

  u32 prev_stack_len = vm->stack.len;
  FuncValue *prev_func = vm->current_func;

  vm->current_func = func;
  vm->current_func_index += 1;

  execute_instrs(vm, &vm->ir->items[func->body_index].instrs);

  if (vm->state == ExecStateReturn)
    vm->state = ExecStateContinue;

  Value *result = NULL;
  if (!value_ignored) {
    if (prev_stack_len == vm->stack.len)
      result = value_unit(vm->current_frame->prev);
    else
      result = value_clone(stack_last(vm), vm->current_frame->prev);
  }

  vm->current_func_index -= 1;
  vm->current_func = prev_func;
  vm->stack.len = prev_stack_len;

  end_frame(vm);

  if (result)
    DA_APPEND(vm->stack, result);
}

void execute_instrs(Vm *vm, Instrs *instrs) {
  Da(Value *) match_stack = {0};
  u32 i = 0;

  while (i < instrs->len) {
    Instr *instr = instrs->items + i;

    switch (instr->kind) {
    case InstrKindString: {
      Value *new_value = value_string(instr->as.string.string, vm->current_frame);
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
      new_func->body_index = instr->as.func.body_index;
      new_func->intrinsic_name = instr->as.func.intrinsic_name;

      if (new_func->intrinsic_name.len == 0) {
        Func *body = vm->ir->items + new_func->body_index;
        catch_vars(&new_func->catched_vars, vm, &body->instrs);
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

      u32 deep = instr->as.func_call.args_len + 1;
      Value *func = vm->stack.items[vm->stack.len - deep];

      if (func->kind != ValueKindFunc) {
        StringBuilder kind_sb = {0};
        sb_push_value(&kind_sb, func, 0, true, false);

        INFO("Stack dump:\n");
        print_stack_dump(&vm->stack);

        if (match_stack.items)
          free(match_stack.items);
        ERROR(META_FMT "Value of type "STR_FMT" is not callable\n",
              META_ARG(instr->meta), STR_ARG(sb_to_str(kind_sb)));
        vm->state = ExecStateExit;
        vm->exit_code = 1;

        free(kind_sb.buffer);

        return;
      }

      if (func->as.func->args.len != instr->as.func_call.args_len)
        PANIC_ARGS(instr->meta, "Expected %u arguments, got %u\n",
                   func->as.func->args.len, instr->as.func_call.args_len);

      for (u32 j = 0; j < func->as.func->catched_vars.len; ++j) {
        Var *catched_var = func->as.func->catched_vars.items + j;

        if (!catched_var->value) {
          Var *var = get_var_from(&vm->frames->vars, catched_var->name);
          if (!var) {
            PANIC_ARGS(instr->meta, "Symbol "STR_FMT" was not found\n",
                       STR_ARG(catched_var->name));
          }

          ++var->value->refs_count;

          catched_var->value = var->value;
        }
      }

      execute_func(vm, func->as.func, &instr->meta, instr->as.func_call.value_ignored);
      if (vm->state == ExecStateExit) {
        if (vm->tracing_enabled && vm->exit_code != 0)
          INFO("Trace: "STR_FMT":%u:%u\n",
               STR_ARG(*instr->meta.file_path),
               instr->meta.row + 1, instr->meta.col + 1);

        return;
      }

      Value *result = stack_last(vm);

      vm->stack.len -= instr->as.func_call.args_len + 1;

      if (!instr->as.func_call.value_ignored)
        vm->stack.items[vm->stack.len - 1] = result;
    } break;

    case InstrKindDefVar: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *last = stack_last(vm);
      ++last->refs_count;

      Var new_var = {
        instr->as.def_var.name,
        last,
      };
      DA_APPEND(vm->current_frame->vars, new_var);

      --vm->stack.len;
    } break;

    case InstrKindGetVar: {
      Var *var = get_var(vm, instr->as.get_var.name);
      if (!var)
        PANIC_ARGS(instr->meta, "Symbol "STR_FMT" was not found\n",
                   STR_ARG(instr->as.get_var.name));

      var->value->parent_var_name = var->name;

      DA_APPEND(vm->stack, var->value);
    } break;

    case InstrKindJump: {
      u32 target = get_instr_index(vm, instr->as.jump.label);
      if (target == (u32) -1)
        PANIC_ARGS(instr->meta, "Target label was not found: "STR_FMT"\n",
                   STR_ARG(instr->as.jump.label));

      i = target - 1;
    } break;

    case InstrKindCondJump: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *cond = stack_last(vm);

      if (value_to_bool(cond)) {
        u32 target = get_instr_index(vm, instr->as.cond_jump.label);
        if (target == (u32) -1)
          PANIC_ARGS(instr->meta, "Target label was not found: "STR_FMT"\n",
                     STR_ARG(instr->as.cond_jump.label));

        i = target - 1;
      }

      --vm->stack.len;
    } break;

    case InstrKindCondNotJump: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *cond = stack_last(vm);

      if (!value_to_bool(cond)) {
        u32 target = get_instr_index(vm, instr->as.cond_not_jump.label);
        if (target == (u32) -1) {
          for (u32 j = 0; j < instrs->len; ++j)
            print_instr(instrs->items + j, false);

          PANIC_ARGS(instr->meta, "Target label was not found: "STR_FMT"\n",
                     STR_ARG(instr->as.cond_not_jump.label));
        }

        i = target - 1;
      }

      --vm->stack.len;
    } break;

    case InstrKindLabel: break;

    case InstrKindMatchBegin: {
      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      DA_APPEND(match_stack, stack_last(vm));

      --vm->stack.len;
    } break;

    case InstrKindMatchCase: {
      if (match_stack.len == 0)
        PANIC(instr->meta, "Match case not inside of a match block\n");

      if (!ensure_stack_len_is_enough(vm, 1, &instr->meta))
        return;

      Value *match_cond = match_stack.items[match_stack.len - 1];
      Value *case_cond = stack_last(vm);

      if (!value_eq(case_cond, match_cond)) {
        u32 target = get_instr_index(vm, instr->as.match_case.not_label);
        if (target == (u32) -1)
          PANIC_ARGS(instr->meta, "Target label not found: "STR_FMT"\n",
                     STR_ARG(instr->as.match_case.not_label));

        i = target - 1;
      }

      --vm->stack.len;
    } break;

    case InstrKindMatchEnd: {
      if (match_stack.len == 0)
        PANIC(instr->meta, "Match end not inside of a match block\n");

      --match_stack.len;
    } break;

    case InstrKindGet: {
      if (!ensure_stack_len_is_enough(vm, instr->as.get.chain_len, &instr->meta))
        return;

      Value *value = vm->stack.items[vm->stack.len - instr->as.get.chain_len];

      for (u32 j = 1; j < instr->as.get.chain_len; ++j) {
        Value *key = vm->stack.items[vm->stack.len - instr->as.get.chain_len + j];

        Value **root = get_child_root(value, key, &instr->meta, vm);
        if (vm->state != ExecStateContinue)
          return;

        value = *root;
      }

      vm->stack.len -= instr->as.get.chain_len;

      DA_APPEND(vm->stack, value);
    } break;

    case InstrKindSet: {
      if (!ensure_stack_len_is_enough(vm, instr->as.set.chain_len + 1, &instr->meta))
        return;

      Value *value = vm->stack.items[vm->stack.len - instr->as.set.chain_len];
      Value **root = NULL;

      for (u32 j = 1; j < instr->as.set.chain_len; ++j) {
        Value *key = vm->stack.items[vm->stack.len - instr->as.set.chain_len + j];

        if (value->refs_count > 1 && !value->is_atom) {
          value = value_clone(value, value->frame);
          if (root)
            *root = value;
        }

        root = get_child_root(value, key, &instr->meta, vm);
        if (vm->state != ExecStateContinue)
          return;

        if (!root)
          break;

        value = *root;
      }

      if (!root) {
        if (value->parent_var_name.len > 0) {
          Var *var = get_var(vm, value->parent_var_name);
          if (var)
            root = &var->value;
        }

        if (!root)
          PANIC(instr->meta, "Value can only be assigned to a left hand side expression\n");
      }

      u32 new_value_offset = vm->stack.len - instr->as.set.chain_len - 1;
      Value *new_value = vm->stack.items[new_value_offset];

      if (new_value->frame == (*root)->frame)
        ++new_value->refs_count;
      else
        new_value = value_clone(new_value, (*root)->frame);

      vm->stack.len -= instr->as.set.chain_len + 1;

      --(*root)->refs_count;
      *root = new_value;
    } break;

    case InstrKindRet: {
      if (match_stack.items)
        free(match_stack.items);
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

        node = node->next;
      }

      vm->stack.len -= instr->as.list.len;

      Value *new_value = value_list(new_list, vm->current_frame);
      DA_APPEND(vm->stack, new_value);
    } break;

    case InstrKindDict: {
      if (!ensure_stack_len_is_enough(vm, instr->as.dict.len * 2, &instr->meta))
        return;

      Dict new_dict = {0};

      for (u32 j = 0; j < instr->as.dict.len; ++j) {
        Value *key = vm->stack.items[vm->stack.len - instr->as.list.len * 2 + j * 2];
        Value *value = vm->stack.items[vm->stack.len - instr->as.list.len * 2 + j * 2 + 1];

        dict_set_value(vm->current_frame, &new_dict, key, value);
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

  if (match_stack.items)
    free(match_stack.items);
}

static void load_labels(Ir *ir, LabelsTables *tables, Arena *arena) {
  tables->len = ir->len;
  tables->cap = tables->len;
  tables->items = arena_alloc(arena, tables->cap * sizeof(LabelsTable));

  for (u32 i = 0; i < ir->len; ++i) {
    LabelsTable table = {0};

    for (u32 j = 0; j < ir->items[i].instrs.len; ++j) {
      Instr *instr = ir->items[i].instrs.items + j;
      if (instr->kind != InstrKindLabel)
        continue;

      u64 index = str_hash(instr->as.label.name) % LABELS_HASH_TABLE_CAP;

      Label *entry = table.items[index];
      while (entry && entry->next)
        entry = entry->next;

      Label *label = arena_alloc(arena, sizeof(Label));
      *label = (Label) {
        instr->as.label.name,
        j, NULL,
      };

      if (entry)
        entry->next = label;
      else
        table.items[index] = label;
    }

    tables->items[i] = table;
  }
}

Value *execute(Vm *vm, Ir *ir, bool value_expected) {
  vm->ir = ir;

  load_labels(ir, &vm->labels, &vm->current_frame->arena);

  execute_instrs(vm, &ir->items[0].instrs);

  if (!value_expected)
    return NULL;

  if (vm->stack.len == 0)
    return value_unit(vm->frames);

  return stack_last(vm);
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
      0, 1, false, {},
    };

    args_end->next = new_arg;
    args_end = new_arg;
  }

  vm_init(&vm, args, intrinsics);

  return vm;
}

void vm_init(Vm *vm, ListNode *args, Intrinsics *intrinsics) {
  intrinsics_append(intrinsics, core_intrinsics, core_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, math_intrinsics, math_intrinsics_len, &vm->frames->arena);
  intrinsics_append(intrinsics, str_intrinsics, str_intrinsics_len, &vm->frames->arena);
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
  StackFrame *frame = vm->frames;
  while (frame) {
    StackFrame *next = frame->next;
    frame_free(frame);
    frame = next;
  }

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

void end_frame(Vm *vm) {
  StackFrame *frame = vm->current_frame;

  for (u32 i = 0; i < frame->values.len; ++i)
    value_free(frame->values.items[i]);
  frame->values.len = 0;

  arena_reset(&frame->arena);

  for (u32 i = 0; i < frame->vars.len; ++i)
    --frame->vars.items[i].value->refs_count;
  frame->vars.len = 0;

  if (vm->current_frame->prev)
    vm->current_frame = vm->current_frame->prev;
}
