#include <unistd.h>

#include "aether/vm.h"

#ifndef EMSCRIPTEN
Value *run_command_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.str.len + 1);
  memcpy(path_cstring, path->as.string.str.ptr, path->as.string.str.len);
  path_cstring[path->as.string.str.len] = '\0';

  i64 exit_code = system(path_cstring);

  free(path_cstring);

  return value_int(exit_code, vm->current_frame);
}

Value *sleep_intrinsic(Vm *vm, Value **args) {
  Value *time = args[0];

  usleep((i64) (time->as._float * 1000000.0));

  return value_unit(vm->current_frame);
}

Value *get_env_intrinsic(Vm *vm, Value **args) {
  Value *var = args[0];

  char *var_cstring = malloc(var->as.string.str.len + 1);
  memcpy(var_cstring, var->as.string.str.ptr, var->as.string.str.len);
  var_cstring[var->as.string.str.len] = '\0';

  char *value = getenv(var_cstring);

  free(var_cstring);

  return value_string(STR(value, strlen(value)), vm->current_frame);
}
#endif

Intrinsic system_intrinsics[] = {
#ifndef EMSCRIPTEN
  { STR_LIT("run-command"), true, 1, { ValueKindString }, &run_command_intrinsic, NULL },
  { STR_LIT("sleep"), false, 1, { ValueKindFloat }, &sleep_intrinsic, NULL },
  { STR_LIT("get-env"), true, 1, { ValueKindString }, &get_env_intrinsic, NULL },
#endif
};

u32 system_intrinsics_len = ARRAY_LEN(system_intrinsics);
