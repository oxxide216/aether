#include "intrinsics.h"
#include "shl_log.h"
#include "shl_arena.h"

static void print_value(Value *value) {
  switch (value->kind) {
  case ValueKindUnit: {
    fputs("unit", stdout);
  } break;

  case ValueKindList: {
    fputc('[', stdout);

    for (u32 i = 0; i < value->as.list.len; ++i) {
      if (i > 0)
        fputs(", ", stdout);

      print_value(value->as.list.items + i);
    }

    fputc(']', stdout);
  } break;

  case ValueKindStrLit: {
    str_print(value->as.str_lit);
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

Value print_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  for (u32 i = 0; i < args->len; ++i) {
    Value value = execute_expr(vm, args->items[i], is_inside_of_func);
    print_value(&value);
  }

  fputc('\n', stdout);

  return (Value) { ValueKindUnit, {} };
}

Value get_args_count_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  (void) args;
  (void) is_inside_of_func;

  return (Value) {
    ValueKindNumber,
    { .number = vm->args.len },
  };
}

Value get_arg_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value index = execute_expr(vm, args->items[0], is_inside_of_func);
  if (index.kind != ValueKindNumber) {
    ERROR("Only numbers can be used for indexing\n");
    exit(1);
  }

  if (index.as.number >= vm->args.len)
    return (Value) { ValueKindUnit, {} };

  return (Value) {
    ValueKindStrLit,
    { .str_lit = vm->args.items[index.as.number] },
  };
}

static void prepare_two_numbers(Value *a, Value *b, char *intrinsic_name, Vm *vm,
                               IrBlock *args, bool is_inside_of_func) {
  *a = execute_expr(vm, args->items[0], is_inside_of_func);
  *b = execute_expr(vm, args->items[1], is_inside_of_func);

  if (a->kind != ValueKindNumber ||
      b->kind != ValueKindNumber) {
    ERROR("%s: wrong argument kinds\n", intrinsic_name);
    exit(1);
  }
}

Value add_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "add", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number + b.as.number },
  };
}

Value sub_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "sub", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number - b.as.number },
  };
}

Value mul_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mul", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number * b.as.number },
  };
}

Value div_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "div", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number / b.as.number },
  };
}

Value mod_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mod", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindNumber,
    { .number = a.as.number % b.as.number },
  };
}

Value eq_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "eq", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number == b.as.number },
  };
}

Value ne_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ne", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number != b.as.number },
  };
}

Value ls_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ls", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number < b.as.number },
  };
}

Value le_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "le", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number <= b.as.number },
  };
}

Value gt_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number > b.as.number },
  };
}

Value ge_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ge", vm, args, is_inside_of_func);

  return (Value) {
    ValueKindBool,
    { ._bool = a.as.number >= b.as.number },
  };
}
