#include "aether/vm.h"

bool str_insert_intrinsic(Vm *vm) {
  Value *index = value_stack_pop(&vm->stack);
  Value *sub_string = value_stack_pop(&vm->stack);
  Value *string = value_stack_pop(&vm->stack);

  Str new_string;
  new_string.len = string->as.string.len + sub_string->as.string.len;
  new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         sub_string->as.string.ptr,
         sub_string->as.string.len);
  memcpy(new_string.ptr + index->as._int + sub_string->as.string.len,
         string->as.string.ptr + index->as._int,
         string->as.string.len - index->as._int);

  value_stack_push_string(&vm->stack, &vm->rc_arena, new_string);

  return true;
}

bool str_remove_intrinsic(Vm *vm) {
  Value *index = value_stack_pop(&vm->stack);
  Value *amount = value_stack_pop(&vm->stack);
  Value *string = value_stack_pop(&vm->stack);

  Str new_string;
  new_string.len = string->as.string.len - amount->as._int;
  new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         string->as.string.ptr + index->as._int + 1,
         string->as.string.len - index->as._int - amount->as._int);

  value_stack_push_string(&vm->stack, &vm->rc_arena, new_string);

  return true;
}

bool str_replace_intrinsic(Vm *vm) {
  Value *index = value_stack_pop(&vm->stack);
  Value *sub_string = value_stack_pop(&vm->stack);
  Value *string = value_stack_pop(&vm->stack);

  Str new_string;
  new_string.len = string->as.string.len - sub_string->as.string.len - index->as._int;
  new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         sub_string->as.string.ptr,
         sub_string->as.string.len);
  memcpy(new_string.ptr + index->as._int + sub_string->as.string.len,
         string->as.string.ptr + index->as._int + sub_string->as.string.len,
         string->as.string.len - index->as._int - sub_string->as.string.len);

  value_stack_push_string(&vm->stack, &vm->rc_arena, new_string);

  return true;
}

bool split_intrinsic(Vm *vm) {
  Value *delimeter = value_stack_pop(&vm->stack);
  Value *string = value_stack_pop(&vm->stack);

  ListNode *list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  ListNode *node = list;
  u32 index = 0, i = 0;

  for (; i < string->as.string.len; ++i) {
    u32 found = true;
    for (u32 j = 0; j + i < string->as.string.len &&
                        j < delimeter->as.string.len; ++j) {
      if (string->as.string.ptr[j + i] != delimeter->as.string.ptr[j]) {
        found = false;
        break;
      }
    }

    if (found) {
      node->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
      node->next->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));

      Str new_string;
      new_string.len = i - index;
      new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);
      memcpy(new_string.ptr, string->as.string.ptr + index, new_string.len);

      *node->next->value = (Value) {
        ValueKindString,
        { .string = new_string },
        0,
      };

      rc_arena_clone(&vm->rc_arena, string->as.string.ptr);

      index = i + 1;
      node = node->next;
    }
  }

  if (i > 0) {
    node->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    node->next->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));

    Str new_string;
    new_string.len = i - index;
    new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);
    memcpy(new_string.ptr, string->as.string.ptr + index, new_string.len);

    *node->next->value = (Value) {
      ValueKindString,
      { .string = new_string },
      0,
    };
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, list);

  return true;
}

bool sub_str_intrinsic(Vm *vm) {
  Value *end = value_stack_pop(&vm->stack);
  Value *begin = value_stack_pop(&vm->stack);
  Value *string = value_stack_pop(&vm->stack);

  if (begin->as._int >= end->as._int ||
      end->as._int > string->as.string.len) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);

    return true;
  }

  Str sub_string = {
    string->as.string.ptr + begin->as._int,
    end->as._int - begin->as._int,
  };

  rc_arena_clone(&vm->rc_arena, string->as.string.ptr);

  value_stack_push_string(&vm->stack, &vm->rc_arena, sub_string);

  return true;
}

bool join_intrinsic(Vm *vm) {
  Value *filler = value_stack_pop(&vm->stack);
  Value *parts = value_stack_pop(&vm->stack);

  StringBuilder sb = {0};

  ListNode *node = parts->as.list->next;
  while (node) {
    if (node != parts->as.list->next)
      sb_push_str(&sb, filler->as.string);

    if (node->value->kind != ValueKindString)
      PANIC("join: wrong part kinds\n");

    sb_push_str(&sb, node->value->as.string);

    node = node->next;
  }

  Str joined = {
    rc_arena_alloc(&vm->rc_arena, sb.len),
    sb.len,
  };

  memcpy(joined.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  value_stack_push_string(&vm->stack, &vm->rc_arena, joined);

  return true;
}

bool eat_str_intrinsic(Vm *vm) {
  Value *pattern = value_stack_pop(&vm->stack);
  Value *string = value_stack_pop(&vm->stack);

  if (string->as.string.len < pattern->as.string.len) {
    value_stack_push_bool(&vm->stack, &vm->rc_arena, false);
    return true;
  }

  Str string_begin = {
    string->as.string.ptr,
    pattern->as.string.len,
  };

  bool matches = str_eq(string_begin, pattern->as.string);

  ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));

  new_list->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  new_list->next->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  *new_list->next->value = (Value) { ValueKindBool, { ._bool = matches }, 0 };

  new_list->next->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  Str new_string = {
    string->as.string.ptr + pattern->as.string.len,
    string->as.string.len - pattern->as.string.len,
  };
  new_list->next->next->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  *new_list->next->next->value = (Value) {
    ValueKindString,
    { .string = new_string },
    0,
  };

  rc_arena_clone(&vm->rc_arena, string->as.string.ptr);

  value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);

  return true;
}

static bool eat_byte(Vm *vm, u32 size) {
  Value *string = value_stack_pop(&vm->stack);

  if (string->as.string.len < size) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  i64 _int = 0;
  switch (size) {
  case 8: _int = *(i64 *) string->as.string.ptr; break;
  case 4: _int = *(i32 *) string->as.string.ptr; break;
  case 2: _int = *(i16 *) string->as.string.ptr; break;
  case 1: _int = *(i8 *) string->as.string.ptr; break;
  }

  ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));

  new_list->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  new_list->next->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  *new_list->next->value = (Value) { ValueKindInt, { ._int = _int }, 0 };

  new_list->next->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  Str new_string = {
    string->as.string.ptr + size,
    string->as.string.len - size,
  };
  new_list->next->next->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  *new_list->next->next->value = (Value) {
    ValueKindString,
    { .string = new_string },
    0,
  };

  rc_arena_clone(&vm->rc_arena, string->as.string.ptr);

  value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);

  return true;
}

bool eat_byte_64_intrinsic(Vm *vm) {
  return eat_byte(vm, 8);
}

bool eat_byte_32_intrinsic(Vm *vm) {
  return eat_byte(vm, 4);
}

bool eat_byte_16_intrinsic(Vm *vm) {
  return eat_byte(vm, 2);
}

bool eat_byte_8_intrinsic(Vm *vm) {
  return eat_byte(vm, 1);
}

Intrinsic str_intrinsics[] = {
  { STR_LIT("str-insert"), true, 3,
    { ValueKindString, ValueKindString, ValueKindInt },
    &str_insert_intrinsic },
  { STR_LIT("str-remove"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &str_remove_intrinsic },
  { STR_LIT("str-replace"), true, 3,
    { ValueKindString, ValueKindString, ValueKindInt },
    &str_replace_intrinsic },
  { STR_LIT("split"), true, 2, { ValueKindString, ValueKindString }, &split_intrinsic },
  { STR_LIT("sub-str"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &sub_str_intrinsic },
  { STR_LIT("join"), true, 2, { ValueKindList, ValueKindString }, &join_intrinsic },
  { STR_LIT("eat-str"), true, 2, { ValueKindString, ValueKindString }, &eat_str_intrinsic },
  { STR_LIT("eat-byte-64"), true, 1, { ValueKindString }, &eat_byte_64_intrinsic },
  { STR_LIT("eat-byte-32"), true, 1, { ValueKindString }, &eat_byte_32_intrinsic },
  { STR_LIT("eat-byte-16"), true, 1, { ValueKindString }, &eat_byte_16_intrinsic },
  { STR_LIT("eat-byte-8"), true, 1, { ValueKindString }, &eat_byte_8_intrinsic },
};

u32 str_intrinsics_len = ARRAY_LEN(str_intrinsics);
