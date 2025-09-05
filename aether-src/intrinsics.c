#include "intrinsics.h"

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
  }
}

Value print_intrinsic(Vm *vm, IrBlock *args) {
  for (u32 i = 0; i < args->len; ++i) {
    Value value = execute_expr(vm, args->items[i]);
    print_value(&value);
  }

  fputc('\n', stdout);

  return (Value) { ValueKindUnit, {} };
}
