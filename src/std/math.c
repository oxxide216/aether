#if defined(__unix__)
#include <math.h>
#elif defined(__wasm__)
#include <emscripten.h>
#endif

#include "aether/vm.h"
#include "aether/misc.h"

Value *abs_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindInt && value->as._int < 0)
    return value_int(-value->as._int, vm->current_frame);
  else if (value->kind == ValueKindFloat && value->as._float < 0.0)
    return value_float(-value->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *min_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_int(a->as._int <= b->as._int ? a->as._int : b->as._int,
                     vm->current_frame);
  else if (b->kind == ValueKindFloat)
    return value_float(a->as._float <= b->as._float ? a->as._float : b->as._float,
                       vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *max_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_float(a->as._int >= b->as._int ? a->as._int : b->as._int,
                       vm->current_frame);
  else if (a->kind == ValueKindFloat)
    return value_float(a->as._float >= b->as._float ? a->as._float : b->as._float,
                       vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *pow_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];
  Value *pow = args[1];

  if (value->kind == ValueKindInt) {
    i64 result = 1;

    for (u32 i = 0; i < (u32) pow->as._int; ++i)
      result *= value->as._int;

    return value_int(result, vm->current_frame);
  } else if (value->kind == ValueKindFloat) {
    f64 result = 1.0;

    for (u32 i = 0; i < (u32) pow->as._int; ++i)
      result *= value->as._float;

    return value_float(result, vm->current_frame);
  }

  return value_unit(vm->current_frame);
}

Value *sqrt_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  return value_float(sqrt(value->as._float), vm->current_frame);
}

Value *round_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  return value_float(round(value->as._float), vm->current_frame);
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
