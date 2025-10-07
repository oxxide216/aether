#include "base/intrinsics.h"
#include "aether-ir/deserializer.h"

bool exit_intrinsic(Vm *vm) {
  Value exit_code = value_stack_pop(&vm->stack);
  if (exit_code.kind != ValueKindInt)
    PANIC("exit: wrong argument kind\n");

  vm->exit_code = exit_code.as._int;
  return false;
}

bool eval_intrinsic(Vm *vm) {
  Value bytecode = value_stack_pop(&vm->stack);
  if (bytecode.kind != ValueKindString)
    PANIC("eval: wrong argument kind\n");

  Ir ir = deserialize((u8 *) bytecode.as.string.str.ptr,
                      bytecode.as.string.str.len,
                      vm->rc_arena);

  i32 argc = 1;
  char *argv[] = { "eval", NULL };
  Intrinsics intrinsics = {0};
  execute(&ir, argc, argv, vm->rc_arena, &intrinsics);

  return true;
}

bool head_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList)
    PANIC("head: wrong argument kind\n");

  if (!value.as.list->next) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  DA_APPEND(vm->stack, value.as.list->next->value);

  return true;
}

bool tail_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList)
    PANIC("tail: wrong argument kind\n");

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  if (value.as.list->next)
    new_list->next = list_clone(vm->rc_arena, value.as.list->next->next);

  value_stack_push_list(&vm->stack, new_list);

  return true;
}

bool last_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList)
    PANIC("last: wrong argument kind\n");

  if (!value.as.list->next) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  ListNode *node = value.as.list->next;
  while (node && node->next)
    node = node->next;

  DA_APPEND(vm->stack, node->value);

  return true;
}

bool nth_intrinsic(Vm *vm) {
  Value index = value_stack_pop(&vm->stack);
  Value list = value_stack_pop(&vm->stack);
  if (list.kind != ValueKindList ||
      index.kind != ValueKindInt)
    PANIC("nth: wrong argument kinds\n");

  ListNode *node = list.as.list->next;
  u32 i = 0;
  while (node && i < index.as._int) {
    node = node->next;
    ++i;
  }

  if (node)
    DA_APPEND(vm->stack, node->value);
  else
    value_stack_push_unit(&vm->stack);

  return true;
}

bool len_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind == ValueKindList) {
    ListNode *node = value.as.list->next;
    u32 len = 0;
    while (node) {
      node = node->next;
      ++len;
    }

    value_stack_push_int(&vm->stack, len);
  } else if (value.kind == ValueKindString) {
    value_stack_push_int(&vm->stack, value.as.string.str.len);
  } else {
    PANIC("len: wrong argument kind\n");
  }

  return true;
}

bool is_empty_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList &&
      value.kind != ValueKindString)
    PANIC("is-empty: wrong argument kind\n");

  if (value.kind == ValueKindList)
    value_stack_push_bool(&vm->stack, value.as.list->next == NULL);
  else
    value_stack_push_bool(&vm->stack, value.as.string.str.len == 0);

  return true;
}

bool get_range_intrinsic(Vm *vm) {
  Value end = value_stack_pop(&vm->stack);
  Value begin = value_stack_pop(&vm->stack);
  Value value = value_stack_pop(&vm->stack);
  if (begin.kind != ValueKindInt ||
      end.kind != ValueKindInt ||
      (value.kind != ValueKindList &&
       value.kind != ValueKindString))
    PANIC("get-range: wrong argument kinds\n");

  DA_APPEND(vm->stack, value);
  len_intrinsic(vm);
  Value len = value_stack_pop(&vm->stack);

  if (begin.as._int < 0 || begin.as._int >= len.as._int ||
      end.as._int <= 0 || end.as._int >= len.as._int ||
      begin.as._int >= end.as._int) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  if (value.kind == ValueKindList) {
    ListNode *node = value.as.list->next;
    ListNode *sub_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < begin.as._int; ++i)
      node = node->next;

    for (u32 i = 0; i < end.as._int - begin.as._int; ++i) {
      sub_list_node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    value_stack_push_list(&vm->stack, sub_list);
  } else {
    Str sub_string = {
      value.as.string.str.ptr + begin.as._int,
      value.as.string.str.len - begin.as._int - end.as._int,
    };

    value_stack_push_string(&vm->stack, sub_string);
  }

  return true;
}

bool map_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList)
    PANIC("map: wrong argument kinds\n");

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list.as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);
    EXECUTE_FUNC(vm, func.as.func.name, 1, true);

    *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    (*new_list_next)->value = value_stack_pop(&vm->stack);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  value_stack_push_list(&vm->stack, new_list);

  return true;
}

bool filter_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList)
    PANIC("filter: wrong argument kinds\n");

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list.as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);
    EXECUTE_FUNC(vm, func.as.func.name, 1, true);

    Value is_ok = value_stack_pop(&vm->stack);
    if (is_ok.kind != ValueKindBool)
      PANIC("filter: wrong argument kinds\n");

    if (is_ok.as._bool) {
      *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      (*new_list_next)->value = node->value;
      new_list_next = &(*new_list_next)->next;
    }

    node = node->next;
  }

  value_stack_push_list(&vm->stack, new_list);

  return true;
}

bool reduce_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value initial_value = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList)
    PANIC("reduce: wrong argument kinds\n");

  Value accumulator = initial_value;
  ListNode *node = list.as.list->next;
  while (node) {
    DA_APPEND(vm->stack, accumulator);
    DA_APPEND(vm->stack, node->value);
    EXECUTE_FUNC(vm, func.as.func.name, 2, true);

    free_value(&accumulator, vm->rc_arena);
    accumulator = value_stack_pop(&vm->stack);
    if (accumulator.kind != initial_value.kind)
      PANIC("reduce: return value's and accumulator's types should be equal\n");

    node = node->next;
  }

  DA_APPEND(vm->stack, accumulator);

  return true;
}

bool split_intrinsic(Vm *vm) {
  Value delimeter = value_stack_pop(&vm->stack);
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString ||
      delimeter.kind != ValueKindString)
    PANIC("split: wrong argument kinds\n");

  ListNode *list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode *node = list;
  u32 index = 0, i = 0;
  for (; i < string.as.string.str.len; ++i) {
    u32 found = true;
    for (u32 j = 0; j + i < string.as.string.str.len &&
                        j < delimeter.as.string.str.len; ++j) {
      if (string.as.string.str.ptr[j + i] != delimeter.as.string.str.ptr[j]) {
        found = false;
        break;
      }
    }

    if (found) {
      node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      node->next->value = (Value) {
        ValueKindString,
        {
          .string = {
            STR(string.as.string.str.ptr + index, i - index),
            (Str *) string.as.string.str.ptr,
          },
        },
      };

      rc_arena_clone(vm->rc_arena, string.as.string.str.ptr);

      index = i + 1;
      node = node->next;
    }
  }

  if (i > 0) {
    node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    node->next->value = (Value) {
      ValueKindString,
      {
        .string = {
          STR(string.as.string.str.ptr + index, i - index),
          (Str *) string.as.string.str.ptr,
        },
      },
    };
  }

  value_stack_push_list(&vm->stack, list);

  return true;
}

bool sub_str_intrinsic(Vm *vm) {
  Value end = value_stack_pop(&vm->stack);
  Value begin = value_stack_pop(&vm->stack);
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString ||
      begin.kind != ValueKindInt ||
      end.kind != ValueKindInt)
    PANIC("sub-str: wrong argument kinds\n");

  if (begin.as._int >= end.as._int ||
      end.as._int > string.as.string.str.len) {
    value_stack_push_unit(&vm->stack);

    return true;
  }

  Str sub_string = {
    string.as.string.str.ptr + begin.as._int,
    end.as._int - begin.as._int,
  };

  rc_arena_clone(vm->rc_arena, string.as.string.str.ptr);

  value_stack_push_string(&vm->stack, sub_string);

  return true;
}

bool eat_str_intrinsic(Vm *vm) {
  Value pattern = value_stack_pop(&vm->stack);
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString ||
      pattern.kind != ValueKindString)
    PANIC("eat-str: wrong argument kinds\n");

  if (string.as.string.str.len < pattern.as.string.str.len) {
    value_stack_push_bool(&vm->stack, false);
    return true;
  }

  Str string_begin = {
    string.as.string.str.ptr,
    pattern.as.string.str.len,
  };

  bool matches = str_eq(string_begin, pattern.as.string.str);

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));

  new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  new_list->next->value = (Value) { ValueKindBool, { ._bool = matches } };

  new_list->next->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  Str new_string = {
    string.as.string.str.ptr + pattern.as.string.str.len,
    string.as.string.str.len - pattern.as.string.str.len,
  };
  new_list->next->next->value = (Value) {
    ValueKindString,
    { .string = { new_string, (Str *) new_string.ptr } },
  };

  rc_arena_clone(vm->rc_arena, string.as.string.str.ptr);

  value_stack_push_list(&vm->stack, new_list);

  return true;
}

static bool eat_byte(Vm *vm, char *intrinsic_name, u32 size) {
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString)
    PANIC("%s: wrong argument kinds\n", intrinsic_name);

  if (string.as.string.str.len < size) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  i64 _int = 0;
  switch (size) {
  case 8: _int = *(i64 *) string.as.string.str.ptr; break;
  case 4: _int = *(i32 *) string.as.string.str.ptr; break;
  case 2: _int = *(i16 *) string.as.string.str.ptr; break;
  case 1: _int = *(i8 *) string.as.string.str.ptr; break;
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));

  new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  new_list->next->value = (Value) { ValueKindInt, { ._int = _int } };

  new_list->next->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  Str new_string = {
    string.as.string.str.ptr + size,
    string.as.string.str.len - size,
  };
  new_list->next->next->value = (Value) {
    ValueKindString,
    { .string = { new_string, (Str *) new_string.ptr } },
  };

  rc_arena_clone(vm->rc_arena, string.as.string.str.ptr);

  value_stack_push_list(&vm->stack, new_list);

  return true;
}

bool eat_byte_64_intrinsic(Vm *vm) {
  return eat_byte(vm, "eat-byte-64", 8);
}

bool eat_byte_32_intrinsic(Vm *vm) {
  return eat_byte(vm, "eat-byte-32", 4);
}

bool eat_byte_16_intrinsic(Vm *vm) {
  return eat_byte(vm, "eat-byte-16", 2);
}

bool eat_byte_8_intrinsic(Vm *vm) {
  return eat_byte(vm, "eat-byte-8", 1);
}

bool str_to_int_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindString)
    PANIC("str-to-int: wrong argument kind\n");

  value_stack_push_int(&vm->stack, str_to_i64(value.as.string.str));

  return true;
}

bool int_to_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindInt)
    PANIC("num-to-str: wrong argument kind\n");

  StringBuilder sb = {0};
  sb_push_i64(&sb, value.as._int);

  Str new_string;
  new_string.len = sb.len;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  value_stack_push_string(&vm->stack, new_string);

  return true;
}

bool bool_to_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool)
    PANIC("bool-to-str: wrong argument kind\n");

  Str string;
  char *cstring;

  if (value.as._bool) {
    string.len = 4;
    cstring = "true";
  } else {
    string.len = 5;
    cstring = "false";
  }

  string.ptr = rc_arena_alloc(vm->rc_arena, string.len);
  memcpy(string.ptr, cstring, string.len);

  value_stack_push_string(&vm->stack, string);

  return true;
}

bool bool_to_int_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool)
    PANIC("bool-to-int: wrong argument kind\n");

  value_stack_push_int(&vm->stack, value.as._bool);

  return true;
}

static bool byte_to_str(Vm *vm, char *intrinsic_name, u32 size) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindInt)
    PANIC("%s: wrong argument kind\n", intrinsic_name);

  Str new_string;
  new_string.len = size;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);

  switch (size) {
  case 8: *(i64 *) new_string.ptr = value.as._int; break;
  case 4: *(i32 *) new_string.ptr = value.as._int; break;
  case 2: *(i16 *) new_string.ptr = value.as._int; break;
  case 1: *(i8 *) new_string.ptr = value.as._int; break;
  }

  value_stack_push_string(&vm->stack, new_string);

  return true;
}

bool byte_64_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-64-to-str", 8);

  return true;
}

bool byte_32_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-32-to-str", 4);

  return true;
}

bool byte_16_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-16-to-str", 2);

  return true;
}

bool byte_8_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-8-to-str", 1);

  return true;
}

bool add_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindInt &&
      b.kind == ValueKindInt) {
    value_stack_push_int(&vm->stack, a.as._int + b.as._int);
  } if (a.kind == ValueKindFloat &&
        b.kind == ValueKindFloat) {
    value_stack_push_float(&vm->stack, a.as._float + b.as._float);
  } else if (a.kind == ValueKindString &&
             b.kind == ValueKindString) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a.as.string.str);
    sb_push_str(&sb, b.as.string.str);

    Str new_string;
    new_string.len = sb.len;
    new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
    memcpy(new_string.ptr, sb.buffer, new_string.len);
    free(sb.buffer);

    value_stack_push_string(&vm->stack, new_string);
  } else if (a.kind == ValueKindList &&
             b.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a.as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node)
      node->next = list_clone(vm->rc_arena, b.as.list->next);

    value_stack_push_list(&vm->stack, new_list);
  } else if (a.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a.as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node) {
      node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      node->next->value = b;
    }

    value_stack_push_list(&vm->stack, new_list);
  } else if (b.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next->value = a;
    new_list->next->next = list_clone(vm->rc_arena, b.as.list->next);

    value_stack_push_list(&vm->stack, new_list);
  } else {
    PANIC("+: wrong argument kinds\n");
  }

  return true;
}

static bool prepare_rwo_numbers(Value *a, Value *b, char *intrinsic_name, Vm *vm) {
  *b = value_stack_pop(&vm->stack);
  *a = value_stack_pop(&vm->stack);

  if ((a->kind != ValueKindInt && a->kind != ValueKindFloat) ||
      (b->kind != ValueKindInt && b->kind != ValueKindFloat))
    PANIC("%s: wrong argument kinds\n", intrinsic_name);

  return true;
}

bool sub_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "-", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_int(&vm->stack, a.as._int - b.as._int);
  else if (a.kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, a.as._float - b.as._float);

  return true;
}

bool mul_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "*", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_int(&vm->stack, a.as._int * b.as._int);
  else if (a.kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, a.as._float * b.as._float);

  return true;
}

bool div_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "/", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_int(&vm->stack, a.as._int / b.as._int);
  else if (a.kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, a.as._float / b.as._float);

  return true;
}

bool mod_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind != ValueKindInt ||
      b.kind != ValueKindInt)
    PANIC("%%: wrong argument kinds\n");

  return true;
}

bool abs_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindInt &&
      value.kind != ValueKindFloat)
    PANIC("abs: wrong argument kind\n");

  if (value.kind == ValueKindInt && value.as._int < 0)
    value.as._int = -value.as._int;
  else if (value.kind == ValueKindFloat && value.as._float < 0)
    value.as._float = -value.as._float;

  DA_APPEND(vm->stack, value);

  return true;
}

bool min_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "min", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_int(&vm->stack, a.as._int <= b.as._int ?
                                     a.as._int :
                                     b.as._int);
  else
    value_stack_push_float(&vm->stack, a.as._float <= b.as._float ?
                                       a.as._float :
                                       b.as._float);

  return true;
}

bool max_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "max", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_int(&vm->stack, a.as._int >= b.as._int ?
                                     a.as._int :
                                     b.as._int);
  else
    value_stack_push_float(&vm->stack, a.as._float >= b.as._float ?
                                       a.as._float :
                                       b.as._float);

  return true;
}

bool eq_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindList &&
      b.kind == ValueKindList)
    value_stack_push_bool(&vm->stack, a.as.list == b.as.list);
  else if (a.kind == ValueKindString &&
           b.kind == ValueKindString)
    value_stack_push_bool(&vm->stack, str_eq(a.as.string.str, b.as.string.str));
  else if (a.kind == ValueKindInt &&
           b.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int == b.as._int);
  else if (a.kind == ValueKindFloat &&
           b.kind == ValueKindFloat)
    value_stack_push_bool(&vm->stack, a.as._float == b.as._float);
  else if (a.kind == ValueKindBool &&
           b.kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, a.as._bool == b.as._bool);
  else
    PANIC("==: wrong argument kinds\n");

  return true;
}

bool ne_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindList &&
      b.kind == ValueKindList)
    value_stack_push_bool(&vm->stack, a.as.list != b.as.list);
  else if (a.kind == ValueKindString &&
           b.kind == ValueKindString)
    value_stack_push_bool(&vm->stack, !str_eq(a.as.string.str, b.as.string.str));
  else if (a.kind == ValueKindInt &&
           b.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int != b.as._int);
  else if (a.kind == ValueKindFloat &&
           b.kind == ValueKindFloat)
    value_stack_push_bool(&vm->stack, a.as._float != b.as._float);
  else if (a.kind == ValueKindBool &&
           b.kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, a.as._bool != b.as._bool);
  else
    PANIC("!=: wrong argument kinds\n");

  return true;
}

bool ls_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "<", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int < b.as._int);
  else
    value_stack_push_bool(&vm->stack, a.as._float < b.as._float);

  return true;
}

bool le_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, "<=", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int <= b.as._int);
  else
    value_stack_push_bool(&vm->stack, a.as._float <= b.as._float);

  return true;
}

bool gt_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, ">", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int > b.as._int);
  else
    value_stack_push_bool(&vm->stack, a.as._float > b.as._float);

  return true;
}

bool ge_intrinsic(Vm *vm) {
  Value a, b;
  prepare_rwo_numbers(&a, &b, ">=", vm);

  if (a.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int >= b.as._int);
  else
    value_stack_push_bool(&vm->stack, a.as._float >= b.as._float);

  return true;
}

static bool value_to_bool(Value *value, char *intrinsic_name, Vm *vm) {
  if (value->kind == ValueKindUnit)
    return false;
  else if (value->kind == ValueKindList)
    return value->as.list->next != NULL;
  else if (value->kind == ValueKindString)
    return value->as.string.str.len != 0;
  else if (value->kind == ValueKindInt)
    return value->as._int != 0;
  else if (value->kind == ValueKindFloat)
    return value->as._float != 0.0;
  else if (value->kind == ValueKindBool)
    return value->as._bool;
  else
    PANIC("%s: wrong argument kinds\n", intrinsic_name);
}

bool and_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindInt &&
      b.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int & b.as._int);
  else if (a.kind == ValueKindBool &&
           b.kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, a.as._bool & b.as._bool);
  else
    PANIC("&: wrong argument kinds\n");

  return true;
}

bool or_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindInt &&
      b.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int | b.as._int);
  else if (a.kind == ValueKindBool &&
           b.kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, a.as._bool | b.as._bool);
  else
    PANIC("|: wrong argument kinds\n");

  return true;
}

bool xor_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindInt &&
      b.kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, a.as._int ^ b.as._int);
  else if (a.kind == ValueKindBool &&
           b.kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, a.as._bool ^ b.as._bool);
  else
    PANIC("^: wrong argument kinds\n");


  return true;
}

bool not_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);

  value_stack_push_bool(&vm->stack, !value_to_bool(&value, "not", vm));

  return true;
}

bool logical_and_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (!value_to_bool(&a, "&&", vm))
    value_stack_push_bool(&vm->stack, false);
  else
    value_stack_push_bool(&vm->stack, value_to_bool(&b, "&&", vm));

  return true;
}

bool logical_or_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (value_to_bool(&a, "||", vm))
    value_stack_push_bool(&vm->stack, true);
  else
    value_stack_push_bool(&vm->stack, value_to_bool(&b, "||", vm));

  return true;
}

bool is_unit_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindUnit);

  return true;
}

bool is_list_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindList);

  return true;
}

bool is_string_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindString);

  return true;
}

bool is_int_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindInt);

  return true;
}

bool is_float_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindFloat);

  return true;
}

bool is_bool_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindBool);

  return true;
}

bool is_fun_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindFunc);

  return true;
}

bool is_record_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindRecord);

  return true;
}

bool type_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);

  switch (value.kind) {
  case ValueKindUnit: {
    value_stack_push_string(&vm->stack, STR_LIT("unit"));
  } break;

  case ValueKindList: {
    value_stack_push_string(&vm->stack, STR_LIT("list"));
  } break;

  case ValueKindString: {
    value_stack_push_string(&vm->stack, STR_LIT("string"));
  } break;

  case ValueKindInt: {
    value_stack_push_string(&vm->stack, STR_LIT("int"));
  } break;

  case ValueKindFloat: {
    value_stack_push_string(&vm->stack, STR_LIT("float"));
  } break;

  case ValueKindBool: {
    value_stack_push_string(&vm->stack, STR_LIT("bool"));
  } break;

  case ValueKindFunc: {
    value_stack_push_string(&vm->stack, STR_LIT("fun"));
  } break;

  case ValueKindRecord: {
    value_stack_push_string(&vm->stack, STR_LIT("record"));
  } break;

  default: {
    PANIC("Unknown type\n");
  }
  }

  return true;
}

Intrinsic base_intrinsics[] = {
  // VM
  { STR_LIT("exit"), 1, false, &exit_intrinsic },
  { STR_LIT("eval"), 1, false, &eval_intrinsic },
  // Base
  { STR_LIT("head"), 1, true, &head_intrinsic },
  { STR_LIT("tail"), 1, true, &tail_intrinsic },
  { STR_LIT("last"), 1, true, &last_intrinsic },
  { STR_LIT("nth"), 2, true, &nth_intrinsic },
  { STR_LIT("len"), 1, true, &len_intrinsic },
  { STR_LIT("is-empty"), 1, true, &is_empty_intrinsic },
  { STR_LIT("get-range"), 3, true, &get_range_intrinsic },
  // Functional stuff
  { STR_LIT("map"), 2, true, &map_intrinsic },
  { STR_LIT("filter"), 2, true, &filter_intrinsic },
  { STR_LIT("reduce"), 3, true, &reduce_intrinsic },
  // String operations
  { STR_LIT("split"), 2, true, &split_intrinsic },
  { STR_LIT("sub-str"), 3, true, &sub_str_intrinsic },
  { STR_LIT("eat-str"), 2, true, &eat_str_intrinsic },
  { STR_LIT("eat-byte-64"), 1, true, &eat_byte_64_intrinsic },
  { STR_LIT("eat-byte-32"), 1, true, &eat_byte_32_intrinsic },
  { STR_LIT("eat-byte-16"), 1, true, &eat_byte_16_intrinsic },
  { STR_LIT("eat-byte-8"), 1, true, &eat_byte_8_intrinsic },
  // Conversions
  { STR_LIT("str-to-int"), 1, true, &str_to_int_intrinsic },
  { STR_LIT("int-to-str"), 1, true, &int_to_str_intrinsic },
  { STR_LIT("bool-to-str"), 1, true, &bool_to_str_intrinsic },
  { STR_LIT("bool-to-int"), 1, true, &bool_to_int_intrinsic },
  // TODO: floating point numbers
  //{ STR_LIT("str-to-float"), 1, true, &str_to_float_intrinsic },
  //{ STR_LIT("float-to-str"), 1, true, &float_to_str_intrinsics },
  { STR_LIT("byte-64-to-str"), 1, true, &byte_64_to_str_intrinsic },
  { STR_LIT("byte-32-to-str"), 1, true, &byte_32_to_str_intrinsic },
  { STR_LIT("byte-16-to-str"), 1, true, &byte_16_to_str_intrinsic },
  { STR_LIT("byte-8-to-str"), 1, true, &byte_8_to_str_intrinsic },
  // Math
  { STR_LIT("+"), 2, true, &add_intrinsic },
  { STR_LIT("-"), 2, true, &sub_intrinsic },
  { STR_LIT("*"), 2, true, &mul_intrinsic },
  { STR_LIT("/"), 2, true, &div_intrinsic },
  { STR_LIT("%"), 2, true, &mod_intrinsic },
  // Advanced math
  { STR_LIT("abs"), 1, true, &abs_intrinsic },
  { STR_LIT("min"), 2, true, &min_intrinsic },
  { STR_LIT("max"), 2, true, &max_intrinsic },
  // TODO: floating point numbers
  // { STR_LIT("sqrt"), 1, true, &sqrt_intrinsic },
  // { STR_LIT("pow"), 2, true, &pow_intrinsic },
  // { STR_LIT("round"), 1, true, &round_intrinsic },
  // Comparisons
  { STR_LIT("=="), 2, true, &eq_intrinsic },
  { STR_LIT("!="), 2, true, &ne_intrinsic },
  { STR_LIT("<"), 2, true, &ls_intrinsic },
  { STR_LIT("<="), 2, true, &le_intrinsic },
  { STR_LIT(">"), 2, true, &gt_intrinsic },
  { STR_LIT(">="), 2, true, &ge_intrinsic },
  // Boolean
  { STR_LIT("&"), 2, true, &and_intrinsic },
  { STR_LIT("|"), 2, true, &or_intrinsic },
  { STR_LIT("^"), 2, true, &xor_intrinsic },
  { STR_LIT("!"), 1, true, &not_intrinsic },
  // Logical
  { STR_LIT("&&"), 2, true, &logical_and_intrinsic },
  { STR_LIT("||"), 2, true, &logical_or_intrinsic },
  // Types
  { STR_LIT("unit?"), 1, true, &is_unit_intrinsic },
  { STR_LIT("list?"), 1, true, &is_list_intrinsic },
  { STR_LIT("string?"), 1, true, &is_string_intrinsic },
  { STR_LIT("int?"), 1, true, &is_int_intrinsic },
  { STR_LIT("float?"), 1, true, &is_float_intrinsic },
  { STR_LIT("bool?"), 1, true, &is_bool_intrinsic },
  { STR_LIT("fun?"), 1, true, &is_fun_intrinsic },
  { STR_LIT("record?"), 1, true, &is_record_intrinsic },
  { STR_LIT("type"), 1, true, &type_intrinsic },
};

u32 base_intrinsics_len = ARRAY_LEN(base_intrinsics);
