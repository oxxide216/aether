#include <unistd.h>

#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_INPUT_BUFFER_SIZE   64

bool got_sigint = false;

static StringBuilder printf_sb = {0};

bool printf_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  printf_sb.len = 0;

  ListNode *node = value->as.list->next;
  while (node) {
    sb_push_value(&printf_sb, node->value, 0);

    node = node->next;
  }

  str_print(STR(printf_sb.buffer, printf_sb.len));
  fflush(stdout);

  return true;
}

bool input_size_intrinsic(Vm *vm) {
  Value *size = value_stack_pop(&vm->stack);

  Str buffer;
  buffer.len = size->as._int;
  buffer.ptr = rc_arena_alloc(&vm->rc_arena, buffer.len);

  if (got_sigint)
    memset(buffer.ptr, 3, buffer.len);
  else
    read(0, buffer.ptr, buffer.len);

  value_stack_push_string(&vm->stack, &vm->rc_arena, buffer);

  return true;
}

bool input_intrinsic(Vm *vm) {
  (void) vm;

  char *buffer = NULL;
  u32 len = 0;

  if (got_sigint) {
    buffer = rc_arena_alloc(&vm->rc_arena, 1);
    buffer[0] = 3;
    len = 1;

    got_sigint = false;
  } else {
    u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
    buffer = rc_arena_alloc(&vm->rc_arena, buffer_size);

    char ch;
    while ((ch = getc(stdin)) != EOF && ch != '\n') {
      if (len >= buffer_size) {
        buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

        char *prev_buffer = buffer;
        buffer = rc_arena_alloc(&vm->rc_arena, buffer_size);
        memcpy(buffer, prev_buffer, len);
        rc_arena_free(&vm->rc_arena, prev_buffer);
      }

      buffer[len++] = ch;
    }
  }

  value_stack_push_string(&vm->stack, &vm->rc_arena, STR(buffer, len));

  return true;
}

bool get_args_intrinsic(Vm *vm) {
  value_stack_push_list(&vm->stack, &vm->rc_arena, vm->args);

  return true;
}

Intrinsic base_intrinsics[] = {
  { STR_LIT("printf"), false, 1, { ValueKindList }, &printf_intrinsic },
  { STR_LIT("input-size"), true, 1, { ValueKindInt }, &input_size_intrinsic },
  { STR_LIT("input"), true, 0, {}, &input_intrinsic },
  { STR_LIT("get-args"), true, 0, {}, &get_args_intrinsic },
};

u32 base_intrinsics_len = ARRAY_LEN(base_intrinsics);
