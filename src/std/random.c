#include <stdlib.h>

#include "aether/vm.h"
#include "aether/misc.h"

Value *seed_intrinsic(Vm *vm, Value **args) {
  Value *seed = args[0];

  srand(seed->as._int);

  return value_unit(vm->current_frame);
}

Value *gen_intrinsic(Vm *vm, Value **args) {
  (void) args;

  return value_float(rand() / RAND_MAX, vm->current_frame);
}

Intrinsic random_intrinsics[] = {
  { STR_LIT("random/seed"), true, 1, { ValueKindString }, &seed_intrinsic, NULL },
  { STR_LIT("random/gen"), true, 1, {}, &gen_intrinsic, NULL },
};

u32 random_intrinsics_len = ARRAY_LEN(random_intrinsics);
