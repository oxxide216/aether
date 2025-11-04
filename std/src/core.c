#include "aether/vm.h"
#include "aether/parser.h"
#include "aether/serializer.h"
#include "aether/deserializer.h"
#include "aether/misc.h"
#include "aether/macros.h"
#include "shl/shl-arena.h"

bool head_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (!value->as.list->next) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  DA_APPEND(vm->stack, value->as.list->next->value);

  return true;
}

bool tail_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (!value->as.list->next) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);

    return true;
  }

  ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  new_list->next = value->as.list->next->next;
  list_use(&vm->rc_arena, new_list->next);

  value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);

  return true;
}

bool last_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (!value->as.list->next) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  ListNode *node = value->as.list->next;
  while (node && node->next)
    node = node->next;

  DA_APPEND(vm->stack, node->value);

  return true;
}

bool get_at_intrinsic(Vm *vm) {
  Value *key = value_stack_pop(&vm->stack);
  Value *collection = value_stack_pop(&vm->stack);

  if (collection->kind == ValueKindList) {
    ListNode *node = collection->as.list->next;
    u32 i = 0;
    while (node && i < key->as._int) {
      node = node->next;
      ++i;
    }

    if (node) {
      DA_APPEND(vm->stack, node->value);

      return true;
    }
  } else if (collection->kind == ValueKindString) {
    if (key->as._int < collection->as.string.len) {
      Str result = collection->as.string;
      result.ptr += key->as._int;
      result.len = 1;

     value_stack_push_string(&vm->stack, &vm->rc_arena, result);

      return true;
    }
  } else if (collection->kind == ValueKindDict) {
    for (u32 i = 0; i < collection->as.dict.len; ++i) {
      if (value_eq(collection->as.dict.items[i].key, key)) {
        DA_APPEND(vm->stack, collection->as.dict.items[i].value);

        return true;
      }
    }
  }

  value_stack_push_unit(&vm->stack, &vm->rc_arena);

  return true;
}

bool set_at_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  Value *key = value_stack_pop(&vm->stack);
  Value *collection = value_stack_pop(&vm->stack);

  if (collection->kind == ValueKindList) {
    ListNode *node = collection->as.list->next;
    u32 i = 0;
    while (node && i < key->as._int) {
      node = node->next;
      ++i;
    }

    if (!node)
      PANIC("set-at: out of bounds\n");

    value_free(node->value, &vm->rc_arena, true);
    node->value = value;
  } else if (collection->kind == ValueKindString) {
    if (key->as._int >= collection->as.string.len)
      PANIC("set-at: out of bounds\n");

    if (value->as.string.len != 1)
      PANIC("set-nth: only one-sized string can be assigned\n");

    collection->as.string.ptr[key->as._int] = value->as.string.ptr[0];
  } else if (collection->kind == ValueKindDict) {
    for (u32 i = 0; i < collection->as.dict.len; ++i) {
      if (value_eq(collection->as.dict.items[i].key, key)) {
        value_free(collection->as.dict.items[i].value, &vm->rc_arena, true);
        collection->as.dict.items[i].value = value;

        return true;
      }
    }

    DictValue dict_value = { key, value };
    DA_APPEND(collection->as.dict, dict_value);
  }

  return true;
}

bool get_index_intrinsic(Vm *vm) {
  Value *item = value_stack_pop(&vm->stack);
  Value *collection = value_stack_pop(&vm->stack);

  if (collection->kind == ValueKindList) {
    ListNode *node = collection->as.list->next;
    u32 i = 0;
    while (node) {
      if (value_eq(node->value, item)) {
        value_stack_push_int(&vm->stack, &vm->rc_arena, i);

        return true;
      }

      node = node->next;
      ++i;
    }
  } else if (collection->kind == ValueKindString) {
    if (item->as.string.len != 1)
      PANIC("get-index: item should be one-sized\n");

    for (u32 i = 0; i < collection->as.string.len; ++i) {
      if (collection->as.string.ptr[i] == item->as.string.ptr[0]) {
        value_stack_push_int(&vm->stack, &vm->rc_arena, i);

        return true;
      }
    }
  }

  value_stack_push_unit(&vm->stack, &vm->rc_arena);

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

    value_stack_push_int(&vm->stack, &vm->rc_arena, len);
  } else if (value->kind == ValueKindString) {
    value_stack_push_int(&vm->stack, &vm->rc_arena, value->as.string.len);
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

  if (begin->as._int < 0)
    begin->as._int = 0;

  if (begin->as._int > len->as._int)
    begin->as._int = len->as._int;

  if (end->as._int > len->as._int)
    end->as._int = len->as._int;

  if (end->as._int < begin->as._int)
    end->as._int = begin->as._int;

  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list->next;
    ListNode *sub_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < begin->as._int; ++i)
      node = node->next;

    for (u32 i = 0; i < end->as._int - begin->as._int; ++i) {
      sub_list_node->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    value_stack_push_list(&vm->stack, &vm->rc_arena, sub_list);
  } else {
    Str sub_string = {
      value->as.string.ptr + begin->as._int,
      end->as._int - begin->as._int,
    };

    value_stack_push_string(&vm->stack, &vm->rc_arena, sub_string);
  }

  return true;
}

bool gen_range_intrinsic(Vm *vm) {
  Value *end = value_stack_pop(&vm->stack);
  Value *begin = value_stack_pop(&vm->stack);

  ListNode *range = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));

  ListNode **next = &range->next;
  for (i64 i = begin->as._int; i < end->as._int; ++i) {
    (*next) = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    (*next)->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
    (*next)->value->kind = ValueKindInt;
    (*next)->value->as._int = i;

    next = &(*next)->next;
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, range);

  return true;
}

bool map_intrinsic(Vm *vm) {
  Value *list = value_stack_pop(&vm->stack);
  Value *func = value_stack_pop(&vm->stack);

  ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);
    // TODO: put real metadata here
    EXECUTE_FUNC(vm, &func->as.func, NULL, true);

    *new_list_next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    (*new_list_next)->value = value_stack_pop(&vm->stack);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);

  return true;
}

bool filter_intrinsic(Vm *vm) {
  Value *list = value_stack_pop(&vm->stack);
  Value *func = value_stack_pop(&vm->stack);

  ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);
    // TODO: put real metadata here
    EXECUTE_FUNC(vm, &func->as.func, NULL, true);

    Value *is_ok = value_stack_pop(&vm->stack);
    if (is_ok->kind != ValueKindBool)
      PANIC("filter: wrong argument kinds\n");

    if (is_ok->as._bool) {
      *new_list_next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
      (*new_list_next)->value = node->value;
      new_list_next = &(*new_list_next)->next;
    }

    node = node->next;
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);

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
    // TODO: put real metadata here
    EXECUTE_FUNC(vm, &func->as.func, NULL, true);

    value_free(accumulator, &vm->rc_arena, true);
    accumulator = value_stack_pop(&vm->stack);

    node = node->next;
  }

  DA_APPEND(vm->stack, accumulator);

  return true;
}

bool value_bigger(Value *a, Value *b) {
  if (a->kind != b->kind)
    return false;

  switch (a->kind) {
  case ValueKindString: {
    Value *min_len_string = a->as.string.len < b->as.string.len ? a : b;
    for (u32 i = 0; i < min_len_string->as.string.len; ++i) {
      if (a->as.string.ptr[i] > b->as.string.ptr[i])
        return true;
      else if (a->as.string.ptr[i] < b->as.string.ptr[i])
        return false;
    }

    return a->as.string.len > b->as.string.len;
  }

  case ValueKindInt: {
    return a->as._int > b->as._int;
  }

  case ValueKindFloat: {
    return a->as._float > b->as._float;
  }

  case ValueKindBool: {
    return a->as._bool > b->as._bool;
  }

  case ValueKindUnit:
  case ValueKindList:
  case ValueKindDict:
  case ValueKindFunc:
  case ValueKindEnv: {
    return false;
  }
  }

  return false;
}

// Shellsort with Ciura gap sequence
bool sort_intrinsic(Vm *vm) {
  Value *list = value_stack_pop(&vm->stack);

  DA_APPEND(vm->stack, list);
  len_intrinsic(vm);
  Value *len = value_stack_pop(&vm->stack);

  Value **sorted = malloc(len->as._int * sizeof(Value *));

  ListNode *node = list->as.list->next;
  u32 i = 0;
  while (node) {
    sorted[i] = node->value;

    node = node->next;
    ++i;
  }

  u32 gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1 };

  for(u32 i = 0; i < ARRAY_LEN(gaps); ++i) {
    for (u32 j = gaps[i]; j < len->as._int; ++j) {
      Value *temp = sorted[j];
      u32 k = j;

      for (; (k >= gaps[i]) && value_bigger(sorted[k - gaps[i]], temp); k -= gaps[i])
        sorted[k] = sorted[k - gaps[i]];

      sorted[k] = temp;
    }
  }

  ListNode *result = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));

  ListNode **next = &result->next;
  for (u32 i = 0; i < len->as._int; ++i) {
    *next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    (*next)->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
    (*next)->value = value_clone(&vm->rc_arena, sorted[i]);
    next = &(*next)->next;
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, result);

  return true;
}

bool to_str_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  StringBuilder sb = {0};
  sb_push_value(&sb, value, 0);

  Str string;
  string.len = sb.len;
  string.ptr = rc_arena_alloc(&vm->rc_arena, sb.len);
  memcpy(string.ptr, sb.buffer, string.len);

  value_stack_push_string(&vm->stack, &vm->rc_arena, string);

  free(sb.buffer);

  return true;
}

static bool byte_to_str(Vm *vm, u32 size) {
  Value *value = value_stack_pop(&vm->stack);

  Str new_string;
  new_string.len = size;
  new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);

  switch (size) {
  case 8: *(i64 *) new_string.ptr = value->as._int; break;
  case 4: *(i32 *) new_string.ptr = value->as._int; break;
  case 2: *(i16 *) new_string.ptr = value->as._int; break;
  case 1: *(i8 *) new_string.ptr = value->as._int; break;
  }

  value_stack_push_string(&vm->stack, &vm->rc_arena, new_string);

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
bool to_int_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (value->kind == ValueKindString) {
    value_stack_push_int(&vm->stack, &vm->rc_arena, str_to_i64(value->as.string));
  } else if (value->kind == ValueKindBool) {
    value_stack_push_int(&vm->stack, &vm->rc_arena, (i64) value->as._bool);
  } else if (value->kind == ValueKindFloat) {
    value_stack_push_int(&vm->stack, &vm->rc_arena, (i64) value->as._float);
  }

  return true;
}

bool to_float_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  if (value->kind == ValueKindInt)
    value_stack_push_float(&vm->stack, &vm->rc_arena, (f64) value->as._int);
  else if (value->kind == ValueKindString)
    value_stack_push_float(&vm->stack, &vm->rc_arena, str_to_f64(value->as.string));

  return true;
}

bool add_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt) {
    value_stack_push_int(&vm->stack, &vm->rc_arena, a->as._int + b->as._int);
  } else if (a->kind == ValueKindFloat &&
        b->kind == ValueKindFloat) {
    value_stack_push_float(&vm->stack, &vm->rc_arena, a->as._float + b->as._float);
  } else if (a->kind == ValueKindString &&
             b->kind == ValueKindString) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a->as.string);
    sb_push_str(&sb, b->as.string);

    Str new_string;
    new_string.len = sb.len;
    new_string.ptr = rc_arena_alloc(&vm->rc_arena, new_string.len);
    memcpy(new_string.ptr, sb.buffer, new_string.len);
    free(sb.buffer);

    value_stack_push_string(&vm->stack, &vm->rc_arena, new_string);
  } else if (a->kind == ValueKindList &&
             b->kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(&vm->rc_arena, a->as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node)
      node->next = list_clone(&vm->rc_arena, b->as.list->next);

    value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);
  } else if (a->kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(&vm->rc_arena, a->as.list->next);
    ListNode *node = new_list;

    while (node && node->next)
      node = node->next;

    node->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    node->next->value = b;
    node->next->next = NULL;

    value_stack_push_list(&vm->stack, &vm->rc_arena, new_list);
  } else if (b->kind == ValueKindList) {
    ListNode *new_list = list_clone(&vm->rc_arena, b->as.list);
    ListNode *next = new_list->next;

    new_list->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
    new_list->next->value = a;
    new_list->next->next = next;

    value_stack_push_list(&vm->stack, &vm->rc_arena, b->as.list);
  }

  return true;
}

bool sub_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_int(&vm->stack, &vm->rc_arena, a->as._int - b->as._int);
  else if (a->kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, &vm->rc_arena, a->as._float - b->as._float);

  return true;
}

bool mul_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt) {
    value_stack_push_int(&vm->stack, &vm->rc_arena, a->as._int * b->as._int);
  } else if (a->kind == ValueKindFloat) {
    value_stack_push_float(&vm->stack, &vm->rc_arena, a->as._float * b->as._float);
  } else if (a->kind == ValueKindString) {
    StringBuilder sb = {0};
    for (u32 i = 0; i < b->as._int; ++i)
      sb_push_str(&sb, a->as.string);

    Str result = {
      rc_arena_alloc(&vm->rc_arena, sb.len),
      sb.len,
    };
    memcpy(result.ptr, sb.buffer, result.len);

    free(sb.buffer);

    value_stack_push_string(&vm->stack, &vm->rc_arena, result);
  }

  return true;
}

bool div_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_int(&vm->stack, &vm->rc_arena, a->as._int / b->as._int);
  else if (a->kind == ValueKindFloat)
    value_stack_push_float(&vm->stack, &vm->rc_arena, a->as._float / b->as._float);

  return true;
}

bool mod_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  value_stack_push_int(&vm->stack, &vm->rc_arena, a->as._int % b->as._int);

  return true;
}

bool eq_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  value_stack_push_bool(&vm->stack, &vm->rc_arena, value_eq(a, b));

  return true;
}

bool ne_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  value_stack_push_bool(&vm->stack, &vm->rc_arena, !value_eq(a, b));

  return true;
}

bool ls_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int < b->as._int);
  else
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._float < b->as._float);

  return true;
}

bool le_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int <= b->as._int);
  else
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._float <= b->as._float);

  return true;
}

bool gt_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int > b->as._int);
  else
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._float > b->as._float);

  return true;
}

bool ge_intrinsic(Vm *vm) {
  Value *a, *b;
  prepare_rwo_numbers(&a, &b, vm);

  if (a->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int >= b->as._int);
  else
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._float >= b->as._float);

  return true;
}

bool and_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int & b->as._int);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._bool & b->as._bool);

  return true;
}

bool or_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int | b->as._int);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._bool | b->as._bool);

  return true;
}

bool xor_intrinsic(Vm *vm) {
  Value *b = value_stack_pop(&vm->stack);
  Value *a = value_stack_pop(&vm->stack);

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._int ^ b->as._int);
  else if (a->kind == ValueKindBool &&
           b->kind == ValueKindBool)
    value_stack_push_bool(&vm->stack, &vm->rc_arena, a->as._bool ^ b->as._bool);


  return true;
}

bool not_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  value_stack_push_bool(&vm->stack, &vm->rc_arena, !value_to_bool(value));

  return true;
}

bool type_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);

  switch (value->kind) {
  case ValueKindUnit: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("unit"));
  } break;

  case ValueKindList: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("list"));
  } break;

  case ValueKindString: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("string"));
  } break;

  case ValueKindInt: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("int"));
  } break;

  case ValueKindFloat: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("float"));
  } break;

  case ValueKindBool: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("bool"));
  } break;

  case ValueKindFunc: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("func"));
  } break;

  case ValueKindDict: {
    value_stack_push_string(&vm->stack, &vm->rc_arena, STR_LIT("dict"));
  } break;

  default: {
    PANIC("Unknown type: %u\n", value->kind);
  }
  }

  return true;
}

bool is_unit_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindUnit);

  return true;
}

bool is_list_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindList);

  return true;
}

bool is_string_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindString);

  return true;
}

bool is_int_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindInt);

  return true;
}

bool is_float_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindFloat);

  return true;
}

bool is_bool_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindBool);

  return true;
}

bool is_func_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindFunc);

  return true;
}

bool is_dict_intrinsic(Vm *vm) {
  Value *value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, &vm->rc_arena, value->kind == ValueKindDict);

  return true;
}

bool make_env_intrinsic(Vm *vm) {
  Intrinsics intrinsics = {0};
  Vm new_vm = vm_create(vm->argc, vm->argv, &intrinsics);

  value_stack_push_env(&vm->stack, &vm->rc_arena, new_vm);

  return true;
}

bool compile_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);
  Value *code = value_stack_pop(&vm->stack);
  Value *env = value_stack_pop(&vm->stack);

  char *path_cstr = malloc(path->as.string.len + 1);
  memcpy(path_cstr, path->as.string.ptr,
         path->as.string.len);
  path_cstr[path->as.string.len] = '\0';

  Ir ir = parse_ex(code->as.string, path_cstr,
                   &env->as.env->macros,
                   &env->as.env->included_files);
  expand_macros_block(&ir, &env->as.env->macros, NULL, NULL, false);

  Str bytecode = {0};
  bytecode.ptr = (char *) serialize(&ir, &bytecode.len);

  value_stack_push_string(&vm->stack, &vm->rc_arena, bytecode);

  free(ir.items);
  free(path_cstr);

  return true;
}

bool eval_compiled_intrinsic(Vm *vm) {
  Value *bytecode = value_stack_pop(&vm->stack);
  Value *env = value_stack_pop(&vm->stack);

  Ir ir = deserialize((u8 *) bytecode->as.string.ptr,
                       bytecode->as.string.len,
                       &vm->rc_arena);

  execute_block(&env->as.env->vm, &ir, true);

  if (env->as.env->vm.stack.len > 0)
    DA_APPEND(vm->stack, env->as.env->vm.stack.items[env->as.env->vm.stack.len - 1]);
  else
    value_stack_push_unit(&vm->stack, &vm->rc_arena);

  return true;
}

bool eval_intrinsic(Vm *vm) {
  Value *code = value_stack_pop(&vm->stack);
  Value *env = value_stack_pop(&vm->stack);

  Ir ir = parse_ex(code->as.string, "eval", &env->as.env->macros,
                   &env->as.env->included_files);
  expand_macros_block(&ir, &env->as.env->macros, NULL, NULL, false);

  execute_block(&env->as.env->vm, &ir, true);

  if (env->as.env->vm.stack.len > 0)
    DA_APPEND(vm->stack, env->as.env->vm.stack.items[env->as.env->vm.stack.len - 1]);
  else
    value_stack_push_unit(&vm->stack, &vm->rc_arena);

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
  { STR_LIT("get-at"), true, 2, { ValueKindList, ValueKindInt }, &get_at_intrinsic },
  { STR_LIT("get-at"), true, 2, { ValueKindString, ValueKindInt }, &get_at_intrinsic },
  { STR_LIT("get-at"), true, 2, { ValueKindDict, ValueKindUnit }, &get_at_intrinsic },
  { STR_LIT("set-at"), false, 3,
    { ValueKindList, ValueKindInt, ValueKindUnit },
    &set_at_intrinsic },
  { STR_LIT("set-at"), false, 3,
    { ValueKindString, ValueKindInt, ValueKindString },
    &set_at_intrinsic },
  { STR_LIT("set-at"), false, 3,
    { ValueKindDict, ValueKindUnit, ValueKindUnit },
    &set_at_intrinsic },
  { STR_LIT("get-index"), true, 2, { ValueKindList, ValueKindUnit }, &get_index_intrinsic },
  { STR_LIT("get-index"), true, 2, { ValueKindString, ValueKindString }, &get_index_intrinsic },
  { STR_LIT("len"), true, 1, { ValueKindList }, &len_intrinsic },
  { STR_LIT("len"), true, 1, { ValueKindString }, &len_intrinsic },
  { STR_LIT("get-range"), true, 3,
    { ValueKindList, ValueKindInt, ValueKindInt },
    &get_range_intrinsic },
  { STR_LIT("get-range"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &get_range_intrinsic },
  { STR_LIT("gen-range"), true, 2, { ValueKindInt, ValueKindInt }, &gen_range_intrinsic },
  { STR_LIT("map"), true, 2, { ValueKindFunc, ValueKindList }, &map_intrinsic },
  { STR_LIT("filter"), true, 2, { ValueKindFunc, ValueKindList }, &filter_intrinsic },
  { STR_LIT("fold"), true, 3,
    { ValueKindFunc, ValueKindUnit, ValueKindList },
    &fold_intrinsic },
  { STR_LIT("sort"), true, 1, { ValueKindList }, &sort_intrinsic },
  // Conversions
  { STR_LIT("to-str"), true, 1, { ValueKindUnit }, &to_str_intrinsic },
  { STR_LIT("byte-64-to-str"), true, 1, { ValueKindInt }, &byte_64_to_str_intrinsic },
  { STR_LIT("byte-32-to-str"), true, 1, { ValueKindInt }, &byte_32_to_str_intrinsic },
  { STR_LIT("byte-16-to-str"), true, 1, { ValueKindInt }, &byte_16_to_str_intrinsic },
  { STR_LIT("byte-8-to-str"), true, 1, { ValueKindInt }, &byte_8_to_str_intrinsic },
  { STR_LIT("to-int"), true, 1, { ValueKindString }, &to_int_intrinsic },
  { STR_LIT("to-int"), true, 1, { ValueKindBool }, &to_int_intrinsic },
  { STR_LIT("to-int"), true, 1, { ValueKindFloat }, &to_int_intrinsic },
  { STR_LIT("to-float"), true, 1, { ValueKindInt }, &to_float_intrinsic },
  { STR_LIT("to-float"), true, 1, { ValueKindString }, &to_float_intrinsic },
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
  // Env
  { STR_LIT("make-env"), true, 0, {}, &make_env_intrinsic },
  { STR_LIT("compile"), true, 3,
    { ValueKindEnv, ValueKindString, ValueKindString },
    &compile_intrinsic },
  { STR_LIT("eval-compiled"), true, 2,
    { ValueKindEnv, ValueKindString },
    &eval_compiled_intrinsic },
  { STR_LIT("eval"), true, 2, { ValueKindEnv, ValueKindString }, &eval_intrinsic },
  // Other
  { STR_LIT("exit"), false, 1, { ValueKindInt }, &exit_intrinsic },
};

u32 core_intrinsics_len = ARRAY_LEN(core_intrinsics);
