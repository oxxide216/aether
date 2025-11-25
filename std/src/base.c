#include <unistd.h>

#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_INPUT_BUFFER_SIZE   64

static StringBuilder printf_sb = {0};

Value *printf_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  printf_sb.len = 0;

  ListNode *node = value->as.list->next;
  while (node) {
    SB_PUSH_VALUE(&printf_sb, node->value, 0, false, vm);

    node = node->next;
  }

  str_print(STR(printf_sb.buffer, printf_sb.len));
  fflush(stdout);

  return value_unit(&vm->arena, &vm->values);
}

Value *input_size_intrinsic(Vm *vm, Value **args) {
  Value *size = args[0];

  Str buffer;
  buffer.len = size->as._int;
  buffer.ptr = arena_alloc(&vm->arena, buffer.len);

  read(0, buffer.ptr, buffer.len);

  return value_string(buffer, &vm->arena, &vm->values);
}

Value *input_intrinsic(Vm *vm, Value **args) {
  (void) args;

  char *buffer = NULL;
  u32 len = 0;

  u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
  buffer = arena_alloc(&vm->arena, buffer_size);

  char ch;
  while ((ch = getc(stdin)) != (char) EOF && ch != '\n') {
    if (len >= buffer_size) {
      buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

      char *prev_buffer = buffer;
      buffer = arena_alloc(&vm->arena, buffer_size);
      memcpy(buffer, prev_buffer, len);
    }

    buffer[len++] = ch;
  }

  return value_string(STR(buffer, len), &vm->arena, &vm->values);
}

Value *get_args_intrinsic(Vm *vm, Value **args) {
  (void) args;

    return value_list(vm->args, &vm->arena, &vm->values);
}

Intrinsic base_intrinsics[] = {
  { STR_LIT("printf"), false, 1, { ValueKindList }, &printf_intrinsic },
  { STR_LIT("input-size"), true, 1, { ValueKindInt }, &input_size_intrinsic },
  { STR_LIT("input"), true, 0, {}, &input_intrinsic },
  { STR_LIT("get-args"), true, 0, {}, &get_args_intrinsic },
};

u32 base_intrinsics_len = ARRAY_LEN(base_intrinsics);
