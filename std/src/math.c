#include <math.h>

#include "aether/vm.h"
#include "aether/misc.h"

Value *abs_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindInt && value->as._int < 0)
    return value_int(&vm->arena, -value->as._int);
  else if (value->kind == ValueKindFloat && value->as._float < 0.0)
    return value_float(&vm->arena, -value->as._float);

  return value_unit(&vm->arena);
}

Value *min_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_int(&vm->arena, a->as._int <=
                                    b->as._int ? a->as._int : b->as._int);
  else if (b->kind == ValueKindFloat)
    return value_float(&vm->arena,
                       a->as._float <=
                       b->as._float ? a->as._float : b->as._float);

  return value_unit(&vm->arena);
}

Value *max_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_float(&vm->arena, a->as._int >=
                                      b->as._int ? a->as._int : b->as._int);
  else if (a->kind == ValueKindFloat)
    return value_float(&vm->arena, a->as._float >=
                                      b->as._float ? a->as._float : b->as._float);

  return value_unit(&vm->arena);
}

Value *pow_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];
  Value *pow = args[1];

  if (value->kind == ValueKindInt) {
    i64 result = 1;

    for (u32 i = 0; i < pow->as._int; ++i)
      result *= value->as._int;

    return value_int(&vm->arena, result);
  } else if (value->kind == ValueKindFloat) {
    f64 result = 1.0;

    for (u32 i = 0; i < pow->as._int; ++i)
      result *= value->as._float;

    return value_float(&vm->arena, result);
  }

  return value_unit(&vm->arena);
}

Value *sqrt_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  return value_float(&vm->arena, sqrt(value->as._float));
}

Value *round_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  return value_float(&vm->arena, round(value->as._float));
}

Intrinsic math_intrinsics[] = {
  { STR_LIT("abs"), true, 1, { ValueKindInt }, &abs_intrinsic },
  { STR_LIT("abs"), true, 1, { ValueKindFloat }, &abs_intrinsic },
  { STR_LIT("min"), true, 1, { ValueKindInt, ValueKindInt }, &min_intrinsic },
  { STR_LIT("min"), true, 1, { ValueKindFloat, ValueKindFloat }, &min_intrinsic },
  { STR_LIT("max"), true, 1, { ValueKindInt, ValueKindInt }, &max_intrinsic },
  { STR_LIT("max"), true, 1, { ValueKindFloat, ValueKindFloat }, &max_intrinsic },
  { STR_LIT("pow"), true, 1, { ValueKindInt, ValueKindInt }, &pow_intrinsic },
  { STR_LIT("pow"), true, 1, { ValueKindFloat, ValueKindInt }, &pow_intrinsic },
  { STR_LIT("sqrt"), true, 1, { ValueKindFloat }, &sqrt_intrinsic },
  { STR_LIT("round"), true, 1, { ValueKindFloat }, &round_intrinsic },
};

u32 math_intrinsics_len = ARRAY_LEN(math_intrinsics);
