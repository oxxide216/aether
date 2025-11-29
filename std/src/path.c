#include <unistd.h>

#include "aether/vm.h"

#define PATH_MAX_LENGTH 64

Value *get_current_path_intrinsic(Vm *vm, Value **args) {
  (void) args;

  char *path = arena_alloc(&vm_get_frame(vm)->arena, PATH_MAX_LENGTH);
  getcwd(path, PATH_MAX_LENGTH);

  return value_string(STR(path, strlen(path)), vm_get_frame(vm), vm->current_frame_index);
}

Value *set_current_path_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  chdir(path_cstring);

  free(path_cstring);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *get_absolute_path_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  char *absolute_path = arena_alloc(&vm_get_frame(vm)->arena, PATH_MAX_LENGTH);
  realpath(path_cstring, absolute_path);

  free(path_cstring);

  return value_string(STR(absolute_path, strlen(absolute_path)),
                      vm_get_frame(vm), vm->current_frame_index);
}

Intrinsic path_intrinsics[] = {
  { STR_LIT("get-current-path"), true, 0, {}, &get_current_path_intrinsic },
  { STR_LIT("set-current-path"), false, 1, { ValueKindString }, &set_current_path_intrinsic },
  { STR_LIT("get-absolute-path"), true, 1, { ValueKindString }, &get_absolute_path_intrinsic },
};

u32 path_intrinsics_len = ARRAY_LEN(path_intrinsics);
