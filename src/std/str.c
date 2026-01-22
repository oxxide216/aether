#include <wctype.h>

#include "aether/vm.h"
#include "aether/misc.h"
#include "lexgen/runtime.h"

Value *str_insert_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *index = args[1];
  Value *sub_string = args[2];

  Str new_string;
  new_string.len = string->as.string.str.len + sub_string->as.string.str.len;
  new_string.ptr = arena_alloc(&vm->current_frame->arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.str.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         sub_string->as.string.str.ptr,
         sub_string->as.string.str.len);
  memcpy(new_string.ptr + index->as._int + sub_string->as.string.str.len,
         string->as.string.str.ptr + index->as._int,
         string->as.string.str.len - index->as._int);

  return value_string(new_string, vm->current_frame);
}

Value *str_remove_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *index = args[1];
  Value *amount = args[2];

  if (index->as._int + amount->as._int > string->as.string.str.len) {
    ERROR("str-remove: out of bounds\n");
    vm->state = ExecStateExit;
    vm->exit_code = 1;
    return value_unit(vm->current_frame);
  }

  Str new_string;
  new_string.len = string->as.string.str.len - amount->as._int;
  new_string.ptr = arena_alloc(&vm->current_frame->arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.str.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         string->as.string.str.ptr + index->as._int,
         string->as.string.str.len - index->as._int - amount->as._int);

  return value_string(new_string, vm->current_frame);
}

Value *str_replace_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *index = args[1];
  Value *sub_string = args[2];

  u32 new_len = string->as.string.str.len;
  if (new_len < index->as._int + sub_string->as.string.str.len)
    new_len = index->as._int + sub_string->as.string.str.len;

  Str new_string;
  new_string.len = new_len;
  new_string.ptr = arena_alloc(&vm->current_frame->arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.str.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         sub_string->as.string.str.ptr,
         sub_string->as.string.str.len);
  memcpy(new_string.ptr + index->as._int + sub_string->as.string.str.len,
         string->as.string.str.ptr + index->as._int + sub_string->as.string.str.len,
         string->as.string.str.len - index->as._int - sub_string->as.string.str.len);

  return value_string(new_string, vm->current_frame);
}

Value *split_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *delimeter = args[1];

  ListNode *list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
  ListNode *node = list;
  u32 index = 0, i = 0;

  for (; i < string->as.string.str.len; ++i) {
    u32 found = true;

    u32 j = 0;

    for (; j + i < string->as.string.str.len &&
               j < delimeter->as.string.str.len; ++j) {
      if (string->as.string.str.ptr[j + i] != delimeter->as.string.str.ptr[j]) {
        found = false;
        break;
      }
    }

    if (j != delimeter->as.string.str.len)
      found = false;

    if (found) {
      node->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      node->next->value = value_alloc(vm->current_frame);

      Str new_string;
      new_string.len = i - index;
      new_string.ptr = arena_alloc(&vm->current_frame->arena, new_string.len);
      memcpy(new_string.ptr, string->as.string.str.ptr + index, new_string.len);

      node->next->value = value_string(new_string, vm->current_frame);

      index = i + delimeter->as.string.str.len;
      node = node->next;
    }
  }

  if (i > 0) {
    node->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    node->next->value = value_alloc(vm->current_frame);

    Str new_string;
    new_string.len = i - index;
    new_string.ptr = arena_alloc(&vm->current_frame->arena, new_string.len);
    memcpy(new_string.ptr, string->as.string.str.ptr + index, new_string.len);

    node->next->value = value_string(new_string, vm->current_frame);
  }

  return value_list(list, vm->current_frame);
}

Value *join_intrinsic(Vm *vm, Value **args) {
  Value *parts = args[0];
  Value *filler = args[1];

  StringBuilder sb = {0};
  bool is_binary = false;

  ListNode *node = parts->as.list->next;
  while (node) {
    if (node != parts->as.list->next)
      sb_push_value(&sb, filler, 0, false, false);

    if (node->value->kind == ValueKindBytes)
      is_binary = true;
    else if (node->value->kind != ValueKindString) {
      ERROR("join: wrong part kinds\n");
      vm->state = ExecStateExit;
      vm->exit_code = 1;
      return value_unit(vm->current_frame);
    }

    sb_push_value(&sb, node->value, 0, false, false);

    node = node->next;
  }

  if (is_binary) {
    Bytes joined = {
      arena_alloc(&vm->current_frame->arena, sb.len),
      sb.len,
    };

    memcpy(joined.ptr, sb.buffer, sb.len);
    free(sb.buffer);

    return value_bytes(joined, vm->current_frame);
  }

  Str joined = {
    arena_alloc(&vm->current_frame->arena, sb.len),
    sb.len,
  };

  memcpy(joined.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  return value_string(joined, vm->current_frame);
}

Value *is_alpha_intrinsic(Vm *vm, Value **args) {
  Value *str = args[0];

  bool is_alpha = true;
  u32 wchar_len;
  wchar _wchar;
  u32 index = 0;

  while((_wchar = get_next_wchar(str->as.string.str, index, &wchar_len)) != '\0') {
    if (!iswalpha(_wchar)) {
      is_alpha = false;

      break;
    }

    index += wchar_len;
  }

  return value_bool(is_alpha, vm->current_frame);
}

Value *is_number_intrinsic(Vm *vm, Value **args) {
  Value *str = args[0];

  bool is_number = true;
  u32 wchar_len;
  wchar _wchar;
  u32 index = 0;

  while((_wchar = get_next_wchar(str->as.string.str, index, &wchar_len)) != '\0') {
    if (!iswdigit(_wchar)) {
      is_number = false;

      break;
    }

    index += wchar_len;
  }

  return value_bool(is_number, vm->current_frame);
}

Value *is_alpha_number_intrinsic(Vm *vm, Value **args) {
  Value *str = args[0];

  bool is_alpha_number = true;
  u32 wchar_len;
  wchar _wchar;
  u32 index = 0;

  while((_wchar = get_next_wchar(str->as.string.str, index, &wchar_len)) != '\0') {
    if (!iswalnum(_wchar)) {
      is_alpha_number = false;

      break;
    }

    index += wchar_len;
  }

  return value_bool(is_alpha_number, vm->current_frame);
}

Value *is_whitespace_intrinsic(Vm *vm, Value **args) {
  Value *str = args[0];

  bool is_whitespace = true;
  u32 wchar_len;
  wchar _wchar;
  u32 index = 0;

  while((_wchar = get_next_wchar(str->as.string.str, index, &wchar_len)) != '\0') {
    if (!iswspace(_wchar)) {
      is_whitespace = false;

      break;
    }

    index += wchar_len;
  }

  return value_bool(is_whitespace, vm->current_frame);
}

Intrinsic str_intrinsics[] = {
  { STR_LIT("str-insert"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindString },
    &str_insert_intrinsic, NULL },
  { STR_LIT("str-remove"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &str_remove_intrinsic, NULL },
  { STR_LIT("str-replace"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindString },
    &str_replace_intrinsic, NULL },
  { STR_LIT("split"), true, 2, { ValueKindString, ValueKindString }, &split_intrinsic, NULL },
  { STR_LIT("join"), true, 2, { ValueKindList, ValueKindString }, &join_intrinsic, NULL },
  { STR_LIT("is-alpha"), true, 1, { ValueKindString }, &is_alpha_intrinsic, NULL },
  { STR_LIT("is-number"), true, 1, { ValueKindString }, &is_number_intrinsic, NULL },
  { STR_LIT("is-alpha-number"), true, 1, { ValueKindString }, &is_alpha_number_intrinsic, NULL },
  { STR_LIT("is-whitespace"), true, 1, { ValueKindString }, &is_whitespace_intrinsic, NULL },
};

u32 str_intrinsics_len = ARRAY_LEN(str_intrinsics);
