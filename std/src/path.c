#include <unistd.h>
#include <linux/limits.h>

#include "aether/vm.h"

Value *get_current_path_intrinsic(Vm *vm, Value **args) {
  (void) args;

  char *path = arena_alloc(&vm->arena, PATH_MAX);
  getcwd(path, PATH_MAX);

  return value_string(STR(path, strlen(path)), &vm->arena, &vm->values);
}

Value *set_current_path_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  chdir(path_cstring);

  free(path_cstring);

  return value_unit(&vm->arena, &vm->values);
}

Value *get_absolute_path_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  char *absolute_path = arena_alloc(&vm->arena, PATH_MAX);
  realpath(path_cstring, absolute_path);

  free(path_cstring);

  return value_string(STR(absolute_path, strlen(absolute_path)),
                      &vm->arena, &vm->values);
}

Intrinsic path_intrinsics[] = {
  { STR_LIT("get-current-path"), true, 0, {}, &get_current_path_intrinsic },
  { STR_LIT("set-current-path"), false, 1, { ValueKindString }, &set_current_path_intrinsic },
  { STR_LIT("get-absolute-path"), true, 1, { ValueKindString }, &get_absolute_path_intrinsic },
};

u32 path_intrinsics_len = ARRAY_LEN(path_intrinsics);
