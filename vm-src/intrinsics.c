#include "intrinsics.h"
#include "shl_log.h"
#include "shl_arena.h"

#define DEFAULT_INPUT_BUFFER_SIZE 64

Intrinsic intrinsics[] = {
  { STR_LIT("print"), (u32) -1, &print_intrinsic },
  { STR_LIT("println"), (u32) -1, &println_intrinsic },
  { STR_LIT("input"), 0, &input_intrinsic },
  { STR_LIT("get-args"), 0, &get_args_intrinsic },
  { STR_LIT("head"), 1, &head_intrinsic },
  { STR_LIT("tail"), 1, &tail_intrinsic },
  { STR_LIT("is-empty"), 1, &is_empty_intrinsic },
  { STR_LIT("str-to-num"), 1, &str_to_num_intrinsic },
  { STR_LIT("num-to-str"), 1, &num_to_str_intrinsic },
  { STR_LIT("bool-to-str"), 1, &bool_to_str_intrinsic },
  { STR_LIT("bool-to-num"), 1, &bool_to_num_intrinsic },
  { STR_LIT("add"), 2, &add_intrinsic },
  { STR_LIT("sub"), 2, &sub_intrinsic },
  { STR_LIT("mul"), 2, &mul_intrinsic },
  { STR_LIT("div"), 2, &div_intrinsic },
  { STR_LIT("mod"), 2, &mod_intrinsic },
  { STR_LIT("eq"), 2, &eq_intrinsic },
  { STR_LIT("ne"), 2, &ne_intrinsic },
  { STR_LIT("ls"), 2, &ls_intrinsic },
  { STR_LIT("le"), 2, &le_intrinsic },
  { STR_LIT("gt"), 2, &gt_intrinsic },
  { STR_LIT("ge"), 2, &ge_intrinsic },
  { STR_LIT("not"), 1, &not_intrinsic },
};

u32 intrinsics_len = ARRAY_LEN(intrinsics);

static void print_value(Value *value) {
  switch (value->kind) {
  case ValueKindUnit: {
    fputs("unit", stdout);
  } break;

  case ValueKindList: {
    fputc('[', stdout);

    ListNode *node = value->as.list;
    while (node) {
      if (node != value->as.list)
        fputc(' ', stdout);

      print_value(&node->value);

      node = node->next;
    }

    fputc(']', stdout);
  } break;

  case ValueKindStr: {
    str_print(value->as.str);
  } break;

  case ValueKindNumber: {
    printf("%ld", value->as.number);
  } break;

  case ValueKindBool: {
    if (value->as._bool)
      fputs("true", stdout);
    else
      fputs("false", stdout);
  } break;
  }
}

Value print_intrinsic(Vm *vm, IrBlock *args) {
  for (u32 i = 0; i < args->len; ++i) {
    Value value = execute_expr(vm, args->items[i]);
    print_value(&value);
  }

  return (Value) { ValueKindUnit, {} };
}

Value println_intrinsic(Vm *vm, IrBlock *args) {
  print_intrinsic(vm, args);
  fputc('\n', stdout);

  return (Value) { ValueKindUnit, {} };
}

Value input_intrinsic(Vm *vm, IrBlock *args) {
  (void) vm;
  (void) args;

  u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
  char *buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
  u32 len = 0;

  char ch;
  while ((ch = getc(stdin)) != EOF && ch != '\n') {
    if (len >= buffer_size) {
      buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

      char *prev_buffer = buffer;
      buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
      memcpy(buffer, prev_buffer, len);
      rc_arena_free(vm->rc_arena, buffer);
    }

    buffer[len++] = ch;
  }

  return (Value) {
    ValueKindStr,
    { .str = { buffer, len } },
  };
}

Value get_args_intrinsic(Vm *vm, IrBlock *args) {
  (void) args;

  return (Value) {
    ValueKindList,
    { .list = vm->args },
  };
}

Value head_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindList) {
    ERROR("head: wrong argument kind\n");
    exit(1);
  }

  if (!value.as.list)
    return (Value) { ValueKindUnit, {} };

  return value.as.list->value;
}

Value tail_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindList) {
    ERROR("tail: wrong argument kind\n");
    exit(1);
  }

  if (!value.as.list)
    return value;

  return (Value) {
    ValueKindList,
    { .list = value.as.list->next },
  };
}

Value is_empty_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindList) {
    ERROR("is-empty: wrong argument kind");
    exit(1);
  }

  return (Value) {
    ValueKindBool,
    { ._bool = value.as.list == NULL },
  };
}

Value str_to_num_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindStr) {
    ERROR("str-to-num: wrong argument kind");
    exit(1);
  }

  return (Value) {
    ValueKindNumber,
    { .number = str_to_i64(value.as.str) },
  };
}

Value num_to_str_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindNumber) {
    ERROR("num-to-str: wrong argument kind");
    exit(1);
  }

  StringBuilder sb = {0};
  sb_push_i64(&sb, value.as.number);

  return (Value) {
    ValueKindStr,
    { .str = sb_to_str(sb) },
  };
}

Value bool_to_str_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindBool) {
    ERROR("bool-to-str: wrong argument kind");
    exit(1);
  }

  Str str;
  char *cstr;

  if (value.as._bool) {
    str.len = 4;
    cstr = "true";
  } else {
    str.len = 5;
    cstr = "false";
  }

  str.ptr = rc_arena_alloc(vm->rc_arena, str.len);
  memcpy(str.ptr, cstr, str.len);

  return (Value) {
    ValueKindStr,
    { .str = str },
  };
}

Value bool_to_num_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);
  if (value.kind != ValueKindBool) {
    ERROR("bool-to-num: wrong argument kind");
    exit(1);
  }

  value.kind = ValueKindNumber;
  return value;
}

static void prepare_two_numbers(Value *a, Value *b, char *intrinsic_name,
                                Vm *vm, IrBlock *args) {
  *a = execute_expr(vm, args->items[0]);
  *b = execute_expr(vm, args->items[1]);

  if (a->kind != ValueKindNumber ||
      b->kind != ValueKindNumber) {
    ERROR("%s: wrong argument kinds\n", intrinsic_name);
    exit(1);
  }
}

Value add_intrinsic(Vm *vm, IrBlock *args) {
  Value a = execute_expr(vm, args->items[0]);
  Value b = execute_expr(vm, args->items[1]);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    return (Value) {
      ValueKindNumber,
      { .number = a.as.number + b.as.number },
    };
  } else if (a.kind == ValueKindStr &&
             b.kind == ValueKindStr) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a.as.str);
    sb_push_str(&sb, b.as.str);

    return (Value) {
      ValueKindStr,
      { .str = sb_to_str(sb) },
    };
  } else {
    ERROR("add: wrong argument kinds\n");
    exit(1);
  }
}

Value sub_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "sub", vm, args);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number - b.as.number },
  };
}

Value mul_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mul", vm, args);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number * b.as.number },
  };
}

Value div_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "div", vm, args);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number / b.as.number },
  };
}

Value mod_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mod", vm, args);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number % b.as.number },
  };
}

Value eq_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "eq", vm, args);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number == b.as.number },
  };
}

Value ne_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ne", vm, args);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number != b.as.number },
  };
}

Value ls_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ls", vm, args);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number < b.as.number },
  };
}

Value le_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "le", vm, args);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number <= b.as.number },
  };
}

Value gt_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm, args);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number > b.as.number },
  };
}

Value ge_intrinsic(Vm *vm, IrBlock *args) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ge", vm, args);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number >= b.as.number },
  };
}

Value not_intrinsic(Vm *vm, IrBlock *args) {
  Value value = execute_expr(vm, args->items[0]);

  if (value.kind != ValueKindBool) {
    ERROR("not: wrong argument kind\n");
    exit(1);
  }

  value.as._bool = !value.as._bool;
  return value;
}
