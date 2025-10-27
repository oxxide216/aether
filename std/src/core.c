#include <unistd.h>

#include "aether/vm.h"
#include "aether/parser.h"
#include "aether/serializer.h"
#include "aether/deserializer.h"
#include "aether/misc.h"

static Value unit_value = { ValueKindUnit, {}, 0 };

bool head_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (!value->as.list->next) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  DA_APPEND(vm->stack, value->as.list->next->value);

  return true;
}

bool tail_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (!value->as.list->next) {
    value_stack_push_unit(&vm->stack);

    return true;
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  new_list->next = value->as.list->next->next;
  list_use(vm->rc_arena, new_list->next);

  value_stack_push_list(&vm->stack, vm->rc_arena, new_list);

  return true;
}

bool last_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (!value->as.list->next) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  ListNode *node = value->as.list->next;
  while (node && node->next)
    node = node->next;

  DA_APPEND(vm->stack, node->value);

  return true;
}

bool nth_intrinsic(Vm *vm) {
  Value *index = value_stack_pop(&vm->stack);
  Value *list = value_stack_pop(&vm->stack);

  ListNode *node = list->as.list->next;
  u32 i = 0;
  while (node && i < index->as._int) {
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
  Value *value = value_stack_pop(&vm->stack);
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list->next;
    u32 len = 0;
    while (node) {
      node = node->next;
      ++len;
    }

    value_stack_push_int(&vm->stack, vm->rc_arena, len);
  } else if (value->kind == ValueKindString) {
    value_stack_push_int(&vm->stack, vm->rc_arena, value->as.string.str.len);
  }

  return true;
}

bool get_range_intrinsic(Vm *vm) {
  Value *end = value_stack_pop(&vm->stack);
  Value *begin = value_stack_pop(&vm->stack);
  Value *value = value_stack_pop(&vm->stack);

  DA_APPEND(vm->stack, value);
  len_intrinsic(vm);
  Value *len = value_stack_pop(&vm->stack);

  if (begin->as._int < 0 || begin->as._int >= len->as._int ||
      end->as._int <= 0 || end->as._int > len->as._int ||
      begin->as._int >= end->as._int) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list->next;
    ListNode *sub_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < begin->as._int; ++i)
      node = node->next;

    for (u32 i = 0; i < end->as._int - begin->as._int; ++i) {
      sub_list_node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    value_stack_push_list(&vm->stack, vm->rc_arena, sub_list);
  } else {
    Str sub_string = {
      value->as.string.str.ptr + begin->as._int,
      end->as._int - begin->as._int,
    };

    value_stack_push_string(&vm->stack, vm->rc_arena, sub_string);
  }

  return true;
}

bool map_intrinsic(Vm *vm) {
  Value *list = value_stack_pop(&vm->stack);
  Value *func = value_stack_pop(&vm->stack);

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);
    EXECUTE_FUNC(vm, &func->as.func, true);

    *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    (*new_list_next)->value = value_stack_pop(&vm->stack);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  value_stack_push_list(&vm->stack, vm->rc_arena, new_list);

  return true;
}

bool filter_intrinsic(Vm *vm) {
  Value *list = value_stack_pop(&vm->stack);
  Value *func = value_stack_pop(&vm->stack);

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);
    EXECUTE_FUNC(vm, &func->as.func, true);

    Value *is_ok = value_stack_pop(&vm->stack);
    if (is_ok->kind != ValueKindBool)
      PANIC("filter: wrong argument kinds\n");

    if (is_ok->as._bool) {
      *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      (*new_list_next)->value = node->value;
      new_list_next = &(*new_list_next)->next;
    }

    node = node->next;
  }

  value_stack_push_list(&vm->stack, vm->rc_arena, new_list);

  return true;
}

bool fold_intrinsic(Vm *vm) {
  Value *list = value_stack_pop(&vm->stack);
  Value *initial_value = value_stack_pop(&vm->stack);
  Value *func = value_stack_pop(&vm->stack);

  Value *accumulator = initial_value;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, accumulator);
    DA_APPEND(vm->stack, node->value);
    EXECUTE_FUNC(vm, &func->as.func, true);

    value_free(accumulator, vm->rc_arena);
    accumulator = value_stack_pop(&vm->stack);

    node = node->next;
  }

  DA_APPEND(vm->stack, accumulator);

  return true;
}

bool str_to_int_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_int(&vm->stack, vm->rc_arena, str_to_i64(value->as.string.str));

  return true;
}

bool str_to_float_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_float(&vm->stack, vm->rc_arena, str_to_f64(value->as.string.str));

  return true;
}

bool int_to_float_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_float(&vm->stack, vm->rc_arena, (f64) value->as._int);

  return true;
}

bool int_to_str_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  StringBuilder sb = {0};
  sb_push_i64(&sb, value->as._int);

  Str new_string;
  new_string.len = sb.len;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  value_stack_push_string(&vm->stack, vm->rc_arena, new_string);

  return true;
}

bool float_to_int_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_int(&vm->stack, vm->rc_arena, (i64) value->as._float);

  return true;
}

bool float_to_str_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  StringBuilder sb = {0};
  sb_push_f64(&sb, value->as._int);

  Str new_string;
  new_string.len = sb.len;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  value_stack_push_string(&vm->stack, vm->rc_arena, new_string);

  return true;
}

bool bool_to_str_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  Str string;
  char *cstring;

  if (value->as._bool) {
    string.len = 4;
    cstring = "true";
  } else {
    string.len = 5;
    cstring = "false";
  }

  string.ptr = rc_arena_alloc(vm->rc_arena, string.len);
  memcpy(string.ptr, cstring, string.len);

  value_stack_push_string(&vm->stack, vm->rc_arena, string);

  return true;
}

bool bool_to_int_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_int(&vm->stack, vm->rc_arena, value->as._bool);

  return true;
}

static bool byte_to_str(Vm *vm, u32 size) {
  Value *value = value_stack_pop(&vm->stack);

  Str new_string;
  new_string.len = size;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);

  switch (size) {
  case 8: *(i64 *) new_string.ptr = value->as._int; break;
  case 4: *(i32 *) new_string.ptr = value->as._int; break;
  case 2: *(i16 *) new_string.ptr = value->as._int; break;
  case 1: *(i8 *) new_string.ptr = value->as._int; break;
  }

  value_stack_push_string(&vm->stack, vm->rc_arena, new_string);

  return true;
}

bool byte_64_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, 8);

  return true;
}

bool byte_32_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, 4);

  return true;
}

bool byte_16_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, 2);

  return true;
}

bool byte_8_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, 1);

  return true;
}

bool add_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt) {
    value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int + b->as._int);
  } else if (a->kind == ValueKindFloat &&
        b->kind == ValueKindFloat) {
    value_stack_push_float(&vm->stack, vm->rc_arena, a->as._float + b->as._float);
  } else if (a->kind == ValueKindString &&
             b->kind == ValueKindString) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a->as.string.str);
    sb_push_str(&sb, b->as.string.str);

    Str new_string;
    new_string.len = sb.len;
    new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
    memcpy(new_string.ptr, sb.buffer, new_string.len);
    free(sb.buffer);

    value_stack_push_string(&vm->stack, vm->rc_arena, new_string);
  } else if (a->kind == ValueKindList &&
             b->kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a->as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node)
      node->next = list_clone(vm->rc_arena, b->as.list->next);

    value_stack_push_list(&vm->stack, vm->rc_arena, new_list);
  } else if (a->kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a->as.list->next);
    ListNode *node = new_list;

    while (node && node->next)
      node = node->next;

    node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    node->next->value = b;
    node->next->next = NULL;

    value_stack_push_list(&vm->stack, vm->rc_arena, new_list);
  } else if (b->kind == ValueKindList) {
    ListNode *new_list = list_clone(vm->rc_arena, b->as.list);
    ListNode *next = new_list->next;

    new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next->value = a;
    new_list->next->next = next;

    value_stack_push_list(&vm->stack, vm->rc_arena, b->as.list);
  }

  return true;
}

bool sub_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int - b->as._int);
  else if (a->kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, vm->rc_arena, a->as._float - b->as._float);

  return true;
}

bool mul_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt) {
    value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int * b->as._int);
  } else if (a->kind == ValueKindFloat) {
    value_stack_push_float(&vm->stack, vm->rc_arena, a->as._float * b->as._float);
  } else if (a->kind == ValueKindString) {
    StringBuilder sb = {0};
    for (u32 i = 0; i < b->as._int; ++i)
      sb_push_str(&sb, a->as.string.str);

    Str result = {
      rc_arena_alloc(vm->rc_arena, sb.len),
      sb.len,
    };
    memcpy(result.ptr, sb.buffer, result.len);

    free(sb.buffer);

    value_stack_push_string(&vm->stack, vm->rc_arena, result);
  }

  return true;
}

bool div_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int / b->as._int);
  else if (a->kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, vm->rc_arena, a->as._float / b->as._float);

  return true;
}

bool mod_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  value_stack_push_int(&vm->stack, vm->rc_arena, a->as._int % b->as._int);

  return true;
}

bool eq_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindList &&
      b->kind == ValueKindList)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as.list->next == b->as.list->next);
  else if (a->kind == ValueKindString &&
           b->kind == ValueKindString)
    value_stack_push_bool(&vm->stack, vm->rc_arena, str_eq(a->as.string.str, b->as.string.str));
  else if (a->kind == ValueKindInt &&
           b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int == b->as._int);
  else if (a->kind == ValueKindFloat &&
           b->kind == ValueKindFloat)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._float == b->as._float);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._bool == b->as._bool);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, false);

  return true;
}

bool ne_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindList &&
      b->kind == ValueKindList)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as.list->next != b->as.list->next);
  else if (a->kind == ValueKindString &&
           b->kind == ValueKindString)
    value_stack_push_bool(&vm->stack, vm->rc_arena, !str_eq(a->as.string.str, b->as.string.str));
  else if (a->kind == ValueKindInt &&
           b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int != b->as._int);
  else if (a->kind == ValueKindFloat &&
           b->kind == ValueKindFloat)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._float != b->as._float);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._bool != b->as._bool);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, true);

  return true;
}

bool ls_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int < b->as._int);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._float < b->as._float);

  return true;
}

bool le_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int <= b->as._int);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._float <= b->as._float);

  return true;
}

bool gt_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int > b->as._int);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._float > b->as._float);

  return true;
}

bool ge_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int >= b->as._int);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._float >= b->as._float);

  return true;
}

bool and_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int & b->as._int);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._bool & b->as._bool);

  return true;
}

bool or_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int | b->as._int);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._bool | b->as._bool);

  return true;
}

bool xor_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._int ^ b->as._int);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, vm->rc_arena, a->as._bool ^ b->as._bool);


  return true;
}

bool not_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_bool(&vm->stack, vm->rc_arena, !value_to_bool(value));

  return true;
}

bool logical_and_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (!value_to_bool(a))
    value_stack_push_bool(&vm->stack, vm->rc_arena, false);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, value_to_bool(b));

  return true;
}

bool logical_or_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (value_to_bool(a))
    value_stack_push_bool(&vm->stack, vm->rc_arena, true);
  else
    value_stack_push_bool(&vm->stack, vm->rc_arena, value_to_bool(b));

  return true;
}

bool type_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  switch (value->kind) {
  case ValueKindUnit: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("unit"));
  } break;

  case ValueKindList: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("list"));
  } break;

  case ValueKindString: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("string"));
  } break;

  case ValueKindInt: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("int"));
  } break;

  case ValueKindFloat: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("float"));
  } break;

  case ValueKindBool: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("bool"));
  } break;

  case ValueKindFunc: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("func"));
  } break;

  case ValueKindDict: {
    value_stack_push_string(&vm->stack, vm->rc_arena, STR_LIT("dict"));
  } break;

  default: {
    PANIC("Unknown type: %u\n", value->kind);
  }
  }

  return true;
}

bool is_unit_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindUnit);

  return true;
}

bool is_list_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindList);

  return true;
}

bool is_string_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindString);

  return true;
}

bool is_int_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindInt);

  return true;
}

bool is_float_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindFloat);

  return true;
}

bool is_bool_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindBool);

  return true;
}

bool is_func_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindFunc);

  return true;
}

bool is_dict_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, vm->rc_arena, value->kind == ValueKindDict);

  return true;
}

#define DEFAULT_INPUT_BUFFER_SIZE   64

static void print_value(ValueStack *stack, Value *value, u32 level) {
  switch (value->kind) {
  case ValueKindUnit: {
    fputs("unit", stdout);
  } break;

  case ValueKindList: {
    fputc('[', stdout);

    ListNode *node = value->as.list->next;
    while (node) {
      if (node != value->as.list->next)
        fputc(' ', stdout);

      if (node->value->kind == ValueKindString)
        fputc('\'', stdout);
      print_value(stack, node->value, level);
      if (node->value->kind == ValueKindString)
        fputc('\'', stdout);

      node = node->next;
    }

    fputc(']', stdout);
  } break;

  case ValueKindString: {
    str_print(value->as.string.str);
  } break;

  case ValueKindInt: {
    printf("%ld", value->as._int);
  } break;

  case ValueKindFloat: {
    printf("%f", value->as._float);
  } break;

  case ValueKindBool: {
    if (value->as._bool)
      fputs("true", stdout);
    else
      fputs("false", stdout);
  } break;

  case ValueKindFunc: {
    fputs("[", stdout);

    for (u32 i = 0; i < value->as.func.args.len; ++i) {
      if (i > 0)
        fputc(' ', stdout);
      str_print(value->as.func.args.items[i]);
    }

    fputs("] -> ...", stdout);
  } break;

  case ValueKindDict: {
    fputs("{\n", stdout);

    for (u32 i = 0; i < value->as.dict.len; ++i) {
      for (u32 j = 0; j < level + 1; ++j)
        fputs("  ", stdout);

      print_value(stack, value->as.dict.items[i].key, level + 1);
      fputs(": ", stdout);
      print_value(stack, value->as.dict.items[i].value, level + 1);

      fputc('\n', stdout);
    }

    for (u32 j = 0; j < level; ++j)
      fputs("  ", stdout);
    fputs("}", stdout);
  } break;
  }
}

bool print_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  print_value(&vm->stack, value, 0);
  fflush(stdout);

  return true;
}

bool input_size_intrinsic(Vm *vm) {
  Value *size = value_stack_pop(&vm->stack);

  Str buffer;
  buffer.len = size->as._int;
  buffer.ptr = rc_arena_alloc(vm->rc_arena, buffer.len);
  read(0, buffer.ptr, buffer.len);

  value_stack_push_string(&vm->stack, vm->rc_arena, buffer);

  return true;
}

bool input_intrinsic(Vm *vm) {
  (void) vm;

  u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
  char *buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
  u32 len = 0;

  char ch;
  while ((ch = getc(stdin)) != EOF && ch != '\n') {
    if (len >= buffer_size) {
      buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

      char *prev_buffer = buffer;
      buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
      memcpy(buffer, prev_buffer, len);
      rc_arena_free(vm->rc_arena, prev_buffer);
    }

    buffer[len++] = ch;
  }

  value_stack_push_string(&vm->stack, vm->rc_arena, STR(buffer, len));

  return true;
}

bool get_args_intrinsic(Vm *vm) {
  value_stack_push_list(&vm->stack, vm->rc_arena, vm->args);

  return true;
}

bool compile_intrinsic(Vm *vm) {
  Value *code = value_stack_pop(&vm->stack);

  Ir ir = parse(code->as.string.str, "eval");
  Str bytecode = {0};
  bytecode.ptr = (char *) serialize(&ir, &bytecode.len);

  value_stack_push_string(&vm->stack, vm->rc_arena, bytecode);

  return true;
}

bool eval_compiled_intrinsic(Vm *vm) {
  Value *bytecode = value_stack_pop(&vm->stack);

  Ir ir = deserialize((u8 *) bytecode->as.string.str.ptr,
                       bytecode->as.string.str.len,
                       vm->rc_arena);

  i32 argc = 1;
  char *argv[] = { "aether", "eval", NULL };
  Intrinsics intrinsics = {0};
  Value *result_value = &unit_value;
  execute(&ir, argc, argv, vm->rc_arena, &intrinsics, &result_value);

  DA_APPEND(vm->stack, result_value);

  return true;
}

bool eval_intrinsic(Vm *vm) {
  Value *code = value_stack_pop(&vm->stack);

  Ir ir = parse(code->as.string.str, "eval");

  i32 argc = 1;
  char *argv[] = { "aether", "eval", NULL };
  Intrinsics intrinsics = {0};
  Value *result_value = &unit_value;
  execute(&ir, argc, argv, vm->rc_arena, &intrinsics, &result_value);

  DA_APPEND(vm->stack, result_value);

  return true;
}

bool exit_intrinsic(Vm *vm) {
  Value *exit_code = value_stack_pop(&vm->stack);

  vm->exit_code = exit_code->as._int;
  return false;
}

Intrinsic core_intrinsics[] = {
  // Base
  { STR_LIT("head"), true, 1, { ValueKindList }, &head_intrinsic },
  { STR_LIT("tail"), true, 1, { ValueKindList }, &tail_intrinsic },
  { STR_LIT("last"), true, 1, { ValueKindList }, &last_intrinsic },
  { STR_LIT("nth"), true, 2, { ValueKindList, ValueKindInt }, &nth_intrinsic },
  { STR_LIT("len"), true, 1, { ValueKindList }, &len_intrinsic },
  { STR_LIT("len"), true, 1, { ValueKindString }, &len_intrinsic },
  { STR_LIT("get-range"), true, 3,
    { ValueKindList, ValueKindInt, ValueKindInt },
    &get_range_intrinsic },
  { STR_LIT("get-range"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &get_range_intrinsic },
  { STR_LIT("map"), true, 2, { ValueKindFunc, ValueKindList }, &map_intrinsic },
  { STR_LIT("filter"), true, 2, { ValueKindFunc, ValueKindList }, &filter_intrinsic },
  { STR_LIT("fold"), true, 3,
    { ValueKindFunc, ValueKindUnit, ValueKindList },
    &fold_intrinsic },
  // Conversions
  { STR_LIT("str-to-int"), true, 1, { ValueKindString }, &str_to_int_intrinsic },
  { STR_LIT("str-to-float"), true, 1, { ValueKindString }, &str_to_float_intrinsic },
  { STR_LIT("int-to-str"), true, 1, { ValueKindInt }, &int_to_str_intrinsic },
  { STR_LIT("int-to-str"), true, 1, { ValueKindInt }, &int_to_str_intrinsic },
  { STR_LIT("bool-to-int"), true, 1, { ValueKindBool }, &bool_to_int_intrinsic },
  { STR_LIT("bool-to-str"), true, 1, { ValueKindBool }, &bool_to_str_intrinsic },
  { STR_LIT("float-to-int"), true, 1, { ValueKindFloat }, &int_to_str_intrinsic },
  { STR_LIT("float-to-str"), true, 1, { ValueKindFloat }, &int_to_str_intrinsic },
  { STR_LIT("byte-64-to-str"), true, 1, { ValueKindInt }, &byte_64_to_str_intrinsic },
  { STR_LIT("byte-32-to-str"), true, 1, { ValueKindInt }, &byte_32_to_str_intrinsic },
  { STR_LIT("byte-16-to-str"), true, 1, { ValueKindInt }, &byte_16_to_str_intrinsic },
  { STR_LIT("byte-8-to-str"), true, 1, { ValueKindInt }, &byte_8_to_str_intrinsic },
  // Math
  { STR_LIT("add"), true, 2, { ValueKindInt, ValueKindInt }, &add_intrinsic },
  { STR_LIT("add"), true, 2, { ValueKindFloat, ValueKindFloat }, &add_intrinsic },
  { STR_LIT("add"), true, 2, { ValueKindString, ValueKindString }, &add_intrinsic },
  { STR_LIT("add"), true, 2, { ValueKindList, ValueKindList }, &add_intrinsic },
  { STR_LIT("add"), true, 2, { ValueKindList, ValueKindUnit }, &add_intrinsic },
  { STR_LIT("add"), true, 2, { ValueKindUnit, ValueKindList }, &add_intrinsic },
  { STR_LIT("sub"), true, 2, { ValueKindInt, ValueKindInt }, &sub_intrinsic },
  { STR_LIT("sub"), true, 2, { ValueKindFloat, ValueKindFloat }, &sub_intrinsic },
  { STR_LIT("mul"), true, 2, { ValueKindInt, ValueKindInt }, &mul_intrinsic },
  { STR_LIT("mul"), true, 2, { ValueKindFloat, ValueKindFloat }, &mul_intrinsic },
  { STR_LIT("mul"), true, 2, { ValueKindString, ValueKindInt }, &mul_intrinsic },
  { STR_LIT("div"), true, 2, { ValueKindInt, ValueKindInt }, &div_intrinsic },
  { STR_LIT("div"), true, 2, { ValueKindFloat, ValueKindFloat }, &div_intrinsic },
  { STR_LIT("mod"), true, 2, { ValueKindInt, ValueKindInt }, mod_intrinsic },
  { STR_LIT("mod"), true, 2, { ValueKindFloat, ValueKindFloat }, &mod_intrinsic },
  // Comparisons
  { STR_LIT("eq"), true, 2, { ValueKindList, ValueKindList }, &eq_intrinsic },
  { STR_LIT("eq"), true, 2, { ValueKindString, ValueKindString }, &eq_intrinsic },
  { STR_LIT("eq"), true, 2, { ValueKindInt, ValueKindInt }, &eq_intrinsic },
  { STR_LIT("eq"), true, 2, { ValueKindFloat, ValueKindFloat }, &eq_intrinsic },
  { STR_LIT("eq"), true, 2, { ValueKindBool, ValueKindBool }, &eq_intrinsic },
  { STR_LIT("ne"), true, 2, { ValueKindList, ValueKindList }, &ne_intrinsic },
  { STR_LIT("ne"), true, 2, { ValueKindString, ValueKindString }, &ne_intrinsic },
  { STR_LIT("ne"), true, 2, { ValueKindInt, ValueKindInt }, &ne_intrinsic },
  { STR_LIT("ne"), true, 2, { ValueKindFloat, ValueKindFloat }, &ne_intrinsic },
  { STR_LIT("ne"), true, 2, { ValueKindBool, ValueKindBool }, &ne_intrinsic },
  { STR_LIT("ls"), true, 2, { ValueKindInt }, &ls_intrinsic },
  { STR_LIT("ls"), true, 2, { ValueKindFloat }, &ls_intrinsic },
  { STR_LIT("le"), true, 2, { ValueKindInt }, &le_intrinsic },
  { STR_LIT("le"), true, 2, { ValueKindFloat }, &le_intrinsic },
  { STR_LIT("gt"), true, 2, { ValueKindInt }, &gt_intrinsic },
  { STR_LIT("gt"), true, 2, { ValueKindFloat }, &gt_intrinsic },
  { STR_LIT("ge"), true, 2, { ValueKindInt }, &ge_intrinsic },
  { STR_LIT("ge"), true, 2, { ValueKindFloat }, &ge_intrinsic },
  // Boolean
  { STR_LIT("and"), true, 2, { ValueKindInt, ValueKindInt }, &and_intrinsic },
  { STR_LIT("and"), true, 2, { ValueKindBool, ValueKindBool }, &and_intrinsic },
  { STR_LIT("or"), true, 2, { ValueKindInt, ValueKindInt }, &or_intrinsic },
  { STR_LIT("or"), true, 2, { ValueKindBool, ValueKindBool }, &or_intrinsic },
  { STR_LIT("and"), true, 2, { ValueKindInt, ValueKindInt }, &xor_intrinsic },
  { STR_LIT("and"), true, 2, { ValueKindBool, ValueKindBool }, &xor_intrinsic },
  { STR_LIT("not"), true, 1, { ValueKindUnit }, &not_intrinsic },
  // Logical
  { STR_LIT("logical-and"), true, 2, { ValueKindUnit, ValueKindUnit }, &logical_and_intrinsic },
  { STR_LIT("logical-or"), true, 2, { ValueKindUnit, ValueKindUnit }, &logical_or_intrinsic },
  // Types
  { STR_LIT("type"), true, 1, { ValueKindUnit }, &type_intrinsic },
  { STR_LIT("is-unit"), true, 1, { ValueKindUnit }, &is_unit_intrinsic },
  { STR_LIT("is-list"), true, 1, { ValueKindUnit }, &is_list_intrinsic },
  { STR_LIT("is-string"), true, 1, { ValueKindUnit }, &is_string_intrinsic },
  { STR_LIT("is-int"), true, 1, { ValueKindUnit }, &is_int_intrinsic },
  { STR_LIT("is-float"), true, 1, { ValueKindUnit }, &is_float_intrinsic },
  { STR_LIT("is-bool"), true, 1, { ValueKindUnit }, &is_bool_intrinsic },
  { STR_LIT("is-func"), true, 1, { ValueKindUnit }, &is_func_intrinsic },
  { STR_LIT("is-dict"), true, 1, { ValueKindUnit }, &is_dict_intrinsic },
  // Base io
  { STR_LIT("print"), false, 1, { ValueKindUnit }, &print_intrinsic },
  { STR_LIT("input-size"), true, 1, { ValueKindInt }, &input_size_intrinsic },
  { STR_LIT("input"), true, 0, {}, &input_intrinsic },
  // Other
  { STR_LIT("get-args"), true, 0, {}, &get_args_intrinsic },
  { STR_LIT("compile"), true, 1, { ValueKindString }, &compile_intrinsic },
  { STR_LIT("eval-compiled"), true, 1, { ValueKindString }, &eval_compiled_intrinsic },
  { STR_LIT("eval"), true, 1, { ValueKindString }, &eval_intrinsic },
  { STR_LIT("exit"), false, 1, { ValueKindInt }, &exit_intrinsic },
};

u32 core_intrinsics_len = ARRAY_LEN(core_intrinsics);
