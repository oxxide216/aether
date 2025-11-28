#include <unistd.h>

#include "aether/vm.h"

Value *run_command_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  i64 exit_code = system(path_cstring);

  free(path_cstring);

  return value_int(exit_code, vm_get_arena(vm), &vm->values);
}

Value *sleep_intrinsic(Vm *vm, Value **args) {
  Value *time = args[0];

  usleep((i64) (time->as._float * 1000000.0));

  return value_unit(vm_get_arena(vm), &vm->values);
}

Intrinsic system_intrinsics[] = {
  { STR_LIT("run-command"), true, 1, { ValueKindString }, &run_command_intrinsic },
  { STR_LIT("sleep"), false, 1, { ValueKindFloat }, &sleep_intrinsic },
};

u32 system_intrinsics_len = ARRAY_LEN(system_intrinsics);
