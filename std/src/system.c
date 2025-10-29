#include <unistd.h>

#include "aether/vm.h"

bool run_command_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  value_stack_push_int(&vm->stack, vm->rc_arena, system(path_cstring));

  free(path_cstring);

  return true;
}

bool sleep_intrinsic(Vm *vm) {
  Value *time = value_stack_pop(&vm->stack);

  usleep((i64) (time->as._float * 1000000.0));

  return true;
}

Intrinsic system_intrinsics[] = {
  { STR_LIT("run-command"), true, 1, { ValueKindString }, &run_command_intrinsic },
  { STR_LIT("sleep"), false, 1, { ValueKindFloat }, &sleep_intrinsic },
};

u32 system_intrinsics_len = ARRAY_LEN(system_intrinsics);
