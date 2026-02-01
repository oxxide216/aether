#include <unistd.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_INPUT_BUFFER_SIZE   64

StringBuilder printf_sb = {0};

Value *printf_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  printf_sb.len = 0;

  u32 len = 0;

  ListNode *node = value->as.list->next;
  while (node) {
    printf("%u\n", len++);

    sb_push_value(&printf_sb, node->value, 0, false, false);

    node = node->next;
  }

#ifdef EMSCRIPTEN
  sb_push_char(&printf_sb, '\0');
  EM_ASM({
    console.log(UTF8ToString($0));
  }, printf_sb.buffer);
#else
  str_print(STR(printf_sb.buffer, printf_sb.len));
  fflush(stdout);
#endif

  return value_unit(vm->current_frame);
}

#ifndef EMSCRIPTEN
Value *input_size_intrinsic(Vm *vm, Value **args) {
  Value *size = args[0];

  Str buffer;
  buffer.len = size->as._int;
  buffer.ptr = arena_alloc(&vm->current_frame->arena, buffer.len);

  if (read(0, buffer.ptr, buffer.len) < 0)
    buffer.len = 0;

  return value_string(buffer, vm->current_frame);
}

Value *input_intrinsic(Vm *vm, Value **args) {
  (void) args;

  char *buffer = NULL;
  u32 len = 0;

  u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
  buffer = arena_alloc(&vm->current_frame->arena, buffer_size);

  char ch;
  while ((ch = getc(stdin)) != (char) EOF && ch != '\n') {
    if (len >= buffer_size) {
      buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

      char *prev_buffer = buffer;
      buffer = arena_alloc(&vm->current_frame->arena, buffer_size);
      memcpy(buffer, prev_buffer, len);
    }

    buffer[len++] = ch;
  }

  return value_string(STR(buffer, len), vm->current_frame);
}

Value *get_args_intrinsic(Vm *vm, Value **args) {
  (void) args;

  Value *result = value_list(vm->args, vm->current_frame);
  result->refs_count = 1;

  return result;
}
#endif

Intrinsic base_intrinsics[] = {
  { STR_LIT("printf"), false, 1, { ValueKindList }, &printf_intrinsic, NULL },
#ifndef EMSCRIPTEN
  { STR_LIT("input-size"), true, 1, { ValueKindInt }, &input_size_intrinsic, NULL },
  { STR_LIT("input"), true, 0, {}, &input_intrinsic, NULL },
  { STR_LIT("get-args"), true, 0, {}, &get_args_intrinsic, NULL },
#endif
};

u32 base_intrinsics_len = ARRAY_LEN(base_intrinsics);
