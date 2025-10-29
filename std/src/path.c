#include <unistd.h>
#include <linux/limits.h>

#include "aether/vm.h"

bool get_current_path_intrinsic(Vm *vm) {
  char *path = rc_arena_alloc(vm->rc_arena, PATH_MAX);
  getcwd(path, PATH_MAX);

  value_stack_push_string(&vm->stack, vm->rc_arena, STR(path, strlen(path)));

  return true;
}

bool set_current_path_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  chdir(path_cstring);

  free(path_cstring);

  return true;
}

bool get_absolute_path_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  char *absolute_path = rc_arena_alloc(vm->rc_arena, PATH_MAX);
  realpath(path_cstring, absolute_path);

  value_stack_push_string(&vm->stack, vm->rc_arena, STR(absolute_path, strlen(absolute_path)));

  free(path_cstring);

  return true;
}

Intrinsic path_intrinsics[] = {
  { STR_LIT("get-current-path"), true, 0, {}, &get_current_path_intrinsic },
  { STR_LIT("set-current-path"), false, 1, { ValueKindString }, &set_current_path_intrinsic },
  { STR_LIT("get-absolute-path"), true, 1, { ValueKindString }, &get_absolute_path_intrinsic },
};

u32 path_intrinsics_len = ARRAY_LEN(path_intrinsics);
