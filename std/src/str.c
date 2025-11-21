#include "aether/vm.h"

Value *str_insert_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *index = args[1];
  Value *sub_string = args[2];

  Str new_string;
  new_string.len = string->as.string.len + sub_string->as.string.len;
  new_string.ptr = arena_alloc(&vm->arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         sub_string->as.string.ptr,
         sub_string->as.string.len);
  memcpy(new_string.ptr + index->as._int + sub_string->as.string.len,
         string->as.string.ptr + index->as._int,
         string->as.string.len - index->as._int);

  return value_string(new_string, &vm->arena, &vm->values);
}

Value *str_remove_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *index = args[1];
  Value *amount = args[2];

  Str new_string;
  new_string.len = string->as.string.len - amount->as._int;
  new_string.ptr = arena_alloc(&vm->arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         string->as.string.ptr + index->as._int + 1,
         string->as.string.len - index->as._int - amount->as._int);

  return value_string(new_string, &vm->arena, &vm->values);
}

Value *str_replace_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *index = args[1];
  Value *sub_string = args[2];

  u32 new_len = string->as.string.len;
  if (new_len < index->as._int + sub_string->as.string.len)
    new_len = index->as._int + sub_string->as.string.len;

  Str new_string;
  new_string.len = new_len;
  new_string.ptr = arena_alloc(&vm->arena, new_string.len);
  memcpy(new_string.ptr, string->as.string.ptr, index->as._int);
  memcpy(new_string.ptr + index->as._int,
         sub_string->as.string.ptr,
         sub_string->as.string.len);
  memcpy(new_string.ptr + index->as._int + sub_string->as.string.len,
         string->as.string.ptr + index->as._int + sub_string->as.string.len,
         string->as.string.len - index->as._int - sub_string->as.string.len);

  return value_string(new_string, &vm->arena, &vm->values);
}

Value *split_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *delimeter = args[1];

  ListNode *list = arena_alloc(&vm->arena, sizeof(ListNode));
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
      node->next = arena_alloc(&vm->arena, sizeof(ListNode));
      node->next->value = arena_alloc(&vm->arena, sizeof(Value));

      Str new_string;
      new_string.len = i - index;
      new_string.ptr = arena_alloc(&vm->arena, new_string.len);
      memcpy(new_string.ptr, string->as.string.ptr + index, new_string.len);

      *node->next->value = (Value) {
        ValueKindString,
        { .string = new_string },
      };

      index = i + 1;
      node = node->next;
    }
  }

  if (i > 0) {
    node->next = arena_alloc(&vm->arena, sizeof(ListNode));
    node->next->value = arena_alloc(&vm->arena, sizeof(Value));

    Str new_string;
    new_string.len = i - index;
    new_string.ptr = arena_alloc(&vm->arena, new_string.len);
    memcpy(new_string.ptr, string->as.string.ptr + index, new_string.len);

    *node->next->value = (Value) {
      ValueKindString,
      { .string = new_string },
    };
  }

  return value_list(list, &vm->arena, &vm->values);
}

Value *sub_str_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *begin = args[1];
  Value *end = args[2];

  if (begin->as._int >= end->as._int ||
      (u32) end->as._int > string->as.string.len)
    return value_unit(&vm->arena, &vm->values);

  Str sub_string = {
    string->as.string.ptr + begin->as._int,
    end->as._int - begin->as._int,
  };

  return value_string(sub_string, &vm->arena, &vm->values);
}

Value *join_intrinsic(Vm *vm, Value **args) {
  Value *parts = args[0];
  Value *filler = args[1];

  StringBuilder sb = {0};

  ListNode *node = parts->as.list->next;
  while (node) {
    if (node != parts->as.list->next)
      sb_push_str(&sb, filler->as.string);

    if (node->value->kind != ValueKindString)
      PANIC(&vm->arena, &vm->values, "join: wrong part kinds\n");

    sb_push_str(&sb, node->value->as.string);

    node = node->next;
  }

  Str joined = {
    arena_alloc(&vm->arena, sb.len),
    sb.len,
  };

  memcpy(joined.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  return value_string(joined, &vm->arena, &vm->values);
}

Value *eat_str_intrinsic(Vm *vm, Value **args) {
  Value *string = args[0];
  Value *pattern = args[1];

  if (string->as.string.len < pattern->as.string.len)
    return value_bool(false, &vm->arena, &vm->values);

  Str string_begin = {
    string->as.string.ptr,
    pattern->as.string.len,
  };

  bool matches = str_eq(string_begin, pattern->as.string);

  ListNode *new_list = arena_alloc(&vm->arena, sizeof(ListNode));

  new_list->next = arena_alloc(&vm->arena, sizeof(ListNode));
  new_list->next->value = arena_alloc(&vm->arena, sizeof(Value));
  *new_list->next->value = (Value) { ValueKindBool, { ._bool = matches } };

  new_list->next->next = arena_alloc(&vm->arena, sizeof(ListNode));
  Str new_string = {
    string->as.string.ptr + pattern->as.string.len,
    string->as.string.len - pattern->as.string.len,
  };
  new_list->next->next->value = arena_alloc(&vm->arena, sizeof(Value));
  *new_list->next->next->value = (Value) {
    ValueKindString,
    { .string = new_string },
  };

  return value_list(new_list, &vm->arena, &vm->values);
}

static Value *eat_byte(Vm *vm, Value **args, u32 size) {
  Value *string = args[0];

  if (string->as.string.len < size)
    return value_unit(&vm->arena, &vm->values);

  i64 _int = 0;
  switch (size) {
  case 8: _int = *(i64 *) string->as.string.ptr; break;
  case 4: _int = *(i32 *) string->as.string.ptr; break;
  case 2: _int = *(i16 *) string->as.string.ptr; break;
  case 1: _int = *(i8 *) string->as.string.ptr; break;
  }

  ListNode *new_list = arena_alloc(&vm->arena, sizeof(ListNode));

  new_list->next = arena_alloc(&vm->arena, sizeof(ListNode));
  new_list->next->value = arena_alloc(&vm->arena, sizeof(Value));
  *new_list->next->value = (Value) { ValueKindInt, { ._int = _int } };

  new_list->next->next = arena_alloc(&vm->arena, sizeof(ListNode));
  Str new_string = {
    string->as.string.ptr + size,
    string->as.string.len - size,
  };
  new_list->next->next->value = arena_alloc(&vm->arena, sizeof(Value));
  *new_list->next->next->value = (Value) {
    ValueKindString,
    { .string = new_string },
  };

  return value_list(new_list, &vm->arena, &vm->values);
}

Value *eat_byte_64_intrinsic(Vm *vm, Value **args) {
  return eat_byte(vm, args, 8);
}

Value *eat_byte_32_intrinsic(Vm *vm, Value **args) {
  return eat_byte(vm, args, 4);
}

Value *eat_byte_16_intrinsic(Vm *vm, Value **args) {
  return eat_byte(vm, args, 2);
}

Value *eat_byte_8_intrinsic(Vm *vm, Value **args) {
  return eat_byte(vm, args, 1);
}

Intrinsic str_intrinsics[] = {
  { STR_LIT("str-insert"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindString },
    &str_insert_intrinsic },
  { STR_LIT("str-remove"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &str_remove_intrinsic },
  { STR_LIT("str-replace"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindString },
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
