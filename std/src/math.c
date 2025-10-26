#include <math.h>

#include "aether/vm.h"
#include "aether/misc.h"

bool abs_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (value->kind == ValueKindInt && value->as._int < 0)
    value->as._int = -value->as._int;
  else if (value->kind == ValueKindFloat && value->as._float < 0)
    value->as._float = -value->as._float;

  DA_APPEND(vm->stack, value);

  return true;
}

bool min_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int <= b->as._int ?
                                     a->as._int :
                                     b->as._int);
  else
    value_stack_push_float(&vm->stack, vm->rc_arena, a->as._float <= b->as._float ?
                                       a->as._float :
                                       b->as._float);

  return true;
}

bool max_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int >= b->as._int ?
                                     a->as._int :
                                     b->as._int);
  else
    value_stack_push_float(&vm->stack, vm->rc_arena, a->as._float >= b->as._float ?
                                       a->as._float :
                                       b->as._float);

  return true;
}

bool pow_intrinsic(Vm *vm) {
  Value *pow = value_stack_pop(&vm->stack);
  Value *value = value_stack_pop(&vm->stack);

  if (value->kind == ValueKindInt) {
    i64 result = 1;

    for (u32 i = 0; i < pow->as._int; ++i)
      result *= value->as._int;

    value_stack_push_int(&vm->stack, vm->rc_arena, result);
  } else {
    f64 result = 1;

    for (u32 i = 0; i < pow->as._int; ++i)
      result *= value->as._float;

    value_stack_push_float(&vm->stack, vm->rc_arena, result);
  }

  return true;
}

bool sqrt_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_float(&vm->stack, vm->rc_arena, sqrt(value->as._float));

  return true;
}

bool round_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_float(&vm->stack, vm->rc_arena, round(value->as._float));

  return true;
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
