#include <unistd.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_INPUT_BUFFER_SIZE   64

#ifdef EMSCRIPTEN
StringBuilder log_sb = {0};
#endif

Value *printf_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  ListNode *node = value->as.list->next;
  while (node) {
    Str string = value_to_str(node->value, false, false, &vm->current_frame->arena);

#ifdef EMSCRIPTEN
    sb_push_str(&log_sb, string);
#else
    str_print(string);
#endif

    node = node->next;
  }

#ifdef EMSCRIPTEN
  Str string = sb_to_str(log_sb);
  EM_ASM({
    console.log(UTF8ToString($0));
  }, string);
  log_sb.len = 0;
#else
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
