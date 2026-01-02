#include <unistd.h>

#include "aether/vm.h"

#define PATH_MAX_LENGTH 64

Value *get_current_path_intrinsic(Vm *vm, Value **args) {
  (void) args;

  char *path = arena_alloc(&vm->current_frame->arena, PATH_MAX_LENGTH);
  getcwd(path, PATH_MAX_LENGTH);

  return value_string(STR(path, strlen(path)), vm->current_frame);
}

Value *set_current_path_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.str.len + 1);
  memcpy(path_cstring, path->as.string.str.ptr, path->as.string.str.len);
  path_cstring[path->as.string.str.len] = '\0';

  chdir(path_cstring);

  free(path_cstring);

  return value_unit(vm->current_frame);
}

Value *get_absolute_path_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = malloc(path->as.string.str.len + 1);
  memcpy(path_cstring, path->as.string.str.ptr, path->as.string.str.len);
  path_cstring[path->as.string.str.len] = '\0';

  char *absolute_path = arena_alloc(&vm->current_frame->arena, PATH_MAX_LENGTH);
  realpath(path_cstring, absolute_path);

  free(path_cstring);

  return value_string(STR(absolute_path, strlen(absolute_path)),
                      vm->current_frame);
}

Intrinsic path_intrinsics[] = {
  { STR_LIT("get-current-path"), true, 0, {}, &get_current_path_intrinsic, NULL },
  { STR_LIT("set-current-path"), false, 1, { ValueKindString }, &set_current_path_intrinsic, NULL },
  { STR_LIT("get-absolute-path"), true, 1, { ValueKindString }, &get_absolute_path_intrinsic, NULL },
};

u32 path_intrinsics_len = ARRAY_LEN(path_intrinsics);
