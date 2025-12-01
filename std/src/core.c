#include "aether/vm.h"
#include "aether/parser.h"
#include "aether/serializer.h"
#include "aether/deserializer.h"
#include "aether/misc.h"
#include "aether/macros.h"
#include "arena.h"

Value *head_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (!value->as.list->next)
    return value_unit(vm_get_frame(vm), vm->current_frame_index);

  return value->as.list->next->value;
}

Value *tail_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (!value->as.list->next)
    return value_unit(vm_get_frame(vm), vm->current_frame_index);

  ListNode *new_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
  new_list->next = value->as.list->next->next;

  return value_list(new_list, vm_get_frame(vm), vm->current_frame_index);
}

Value *last_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (!value->as.list->next)
    return value_unit(vm_get_frame(vm), vm->current_frame_index);

  ListNode *node = value->as.list->next;
  while (node && node->next)
    node = node->next;

  return node->value;
}

Value *get_index_intrinsic(Vm *vm, Value **args) {
  Value *collection = args[0];
  Value *item = args[1];

  if (collection->kind == ValueKindList) {
    ListNode *node = collection->as.list->next;
    u32 i = 0;
    while (node) {
      if (value_eq(node->value, item))
        return value_int(i, vm_get_frame(vm), vm->current_frame_index);

      node = node->next;
      ++i;
    }
  } else if (collection->kind == ValueKindString) {
    if (item->as.string.len != 1)
      PANIC(vm_get_frame(vm), vm->current_frame_index, "get-index: item should be one-sized\n");

    for (u32 i = 0; i < collection->as.string.len; ++i)
      if (collection->as.string.ptr[i] == item->as.string.ptr[0])
        return value_int(i, vm_get_frame(vm), vm->current_frame_index);
  }

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *len_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list->next;
    u32 len = 0;
    while (node) {
      node = node->next;
      ++len;
    }

    return value_int(len, vm_get_frame(vm), vm->current_frame_index);
  } else if (value->kind == ValueKindString) {
    return value_int(value->as.string.len, vm_get_frame(vm), vm->current_frame_index);
  }

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *get_range_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];
  Value *begin = args[1];
  Value *end = args[2];

  Value *len = len_intrinsic(vm, &value);

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
    ListNode *sub_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < (u32) begin->as._int; ++i)
      node = node->next;

    for (u32 i = 0; i < (u32) end->as._int - begin->as._int; ++i) {
      sub_list_node->next = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    return value_list(sub_list, vm_get_frame(vm), vm->current_frame_index);
  } else {
    Str sub_string = {
      value->as.string.ptr + begin->as._int,
      end->as._int - begin->as._int,
    };

    return value_string(sub_string, vm_get_frame(vm), vm->current_frame_index);
  }

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *gen_range_intrinsic(Vm *vm, Value **args) {
  Value *begin = args[0];
  Value *end = args[1];

  ListNode *range = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));

  ListNode **next = &range->next;
  for (i64 i = begin->as._int; i < end->as._int; ++i) {
    (*next) = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    (*next)->value = value_alloc(vm_get_frame(vm));
    (*next)->value->kind = ValueKindInt;
    (*next)->value->as._int = i;

    next = &(*next)->next;
  }

  return value_list(range, vm_get_frame(vm), vm->current_frame_index);
}

Value *map_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *list = args[1];

  ListNode *new_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list->as.list->next;
  while (node) {
    Value *func_args[] = { node->value };
    // TODO: put real metadata here
    Value *replacement = execute_func(vm, func_args, &func->as.func, NULL, true);
    if (vm->state != ExecStateContinue)
      break;

    *new_list_next = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    (*new_list_next)->value = replacement;
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  return value_list(new_list, vm_get_frame(vm), vm->current_frame_index);
}

Value *filter_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *list = args[1];

  ListNode *new_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list->as.list->next;
  while (node) {
    Value *func_args[] = { node->value };
    // TODO: put real metadata here
    Value *is_ok = execute_func(vm, func_args, &func->as.func, NULL, true);
    if (vm->state != ExecStateContinue)
      break;

    if (is_ok->kind != ValueKindBool)
      PANIC(vm_get_frame(vm), vm->current_frame_index, "filter: wrong argument kinds\n");

    if (is_ok->as._bool) {
      *new_list_next = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
      (*new_list_next)->value = node->value;
      new_list_next = &(*new_list_next)->next;
    }

    node = node->next;
  }

  return value_list(new_list, vm_get_frame(vm), vm->current_frame_index);
}

Value *fold_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *initial_value = args[1];
  Value *list = args[2];

  Value *accumulator = initial_value;
  ListNode *node = list->as.list->next;
  while (node) {
    Value *func_args[] = { accumulator, node->value };
    // TODO: put real metadata here
    Value *new_accumulator = execute_func(vm, func_args, &func->as.func, NULL, true);
    if (vm->state != ExecStateContinue)
      break;

    accumulator = new_accumulator;

    node = node->next;
  }

  return accumulator;
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
Value *sort_intrinsic(Vm *vm, Value **args) {
  Value *list = args[0];

  Value *len = len_intrinsic(vm, args);
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
    for (u32 j = gaps[i]; j < (u32) len->as._int; ++j) {
      Value *temp = sorted[j];
      u32 k = j;

      for (; (k >= gaps[i]) && value_bigger(sorted[k - gaps[i]], temp); k -= gaps[i])
        sorted[k] = sorted[k - gaps[i]];

      sorted[k] = temp;
    }
  }

  ListNode *result = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));

  ListNode **next = &result->next;
  for (u32 i = 0; i < (u32) len->as._int; ++i) {
    *next = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    (*next)->value = value_clone(sorted[i], vm_get_frame(vm), vm->current_frame_index);
    next = &(*next)->next;
  }

  free(sorted);

  return value_list(result, vm_get_frame(vm), vm->current_frame_index);
}

Value *to_str_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  StringBuilder sb = {0};
  SB_PUSH_VALUE(&sb, value, 0, false, vm);

  Str string;
  string.len = sb.len;
  string.ptr = arena_alloc(&vm_get_frame(vm)->arena, sb.len);
  memcpy(string.ptr, sb.buffer, string.len);

  free(sb.buffer);

  return value_string(string, vm_get_frame(vm), vm->current_frame_index);
}

static Value *byte_to_str(Vm *vm, Value *value, u32 size) {
  Str new_string;
  new_string.len = size;
  new_string.ptr = arena_alloc(&vm_get_frame(vm)->arena, new_string.len);

  switch (size) {
  case 8: *(i64 *) new_string.ptr = value->as._int; break;
  case 4: *(i32 *) new_string.ptr = value->as._int; break;
  case 2: *(i16 *) new_string.ptr = value->as._int; break;
  case 1: *(i8 *) new_string.ptr = value->as._int; break;
  }

  return value_string(new_string, vm_get_frame(vm), vm->current_frame_index);
}

Value *byte_64_to_str_intrinsic(Vm *vm, Value **args) {
  return byte_to_str(vm, args[0], 8);
}

Value *byte_32_to_str_intrinsic(Vm *vm, Value **args) {
  return byte_to_str(vm, args[0], 4);
}

Value *byte_16_to_str_intrinsic(Vm *vm, Value **args) {
  return byte_to_str(vm, args[0], 2);
}

Value *byte_8_to_str_intrinsic(Vm *vm, Value **args) {
  return byte_to_str(vm, args[0], 1);
}

Value *to_int_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindString) {
    return value_int(str_to_i64(value->as.string), vm_get_frame(vm), vm->current_frame_index);
  } else if (value->kind == ValueKindBool) {
    return value_int((i64) value->as._bool, vm_get_frame(vm), vm->current_frame_index);
  } else if (value->kind == ValueKindFloat) {
    return value_int((i64) value->as._float, vm_get_frame(vm), vm->current_frame_index);
  }

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *to_float_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindInt)
    return value_float((f64) value->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (value->kind == ValueKindString)
    return value_float(str_to_f64(value->as.string), vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *add_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt) {
    return value_int(a->as._int + b->as._int, vm_get_frame(vm), vm->current_frame_index);
  } else if (a->kind == ValueKindFloat &&
             b->kind == ValueKindFloat) {
    return value_float(a->as._float + b->as._float, vm_get_frame(vm), vm->current_frame_index);
  } else if (a->kind == ValueKindString &&
             b->kind == ValueKindString) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a->as.string);
    sb_push_str(&sb, b->as.string);

    Str new_string;
    new_string.len = sb.len;
    new_string.ptr = arena_alloc(&vm_get_frame(vm)->arena, new_string.len);
    memcpy(new_string.ptr, sb.buffer, new_string.len);
    free(sb.buffer);

    return value_string(new_string, vm_get_frame(vm), vm->current_frame_index);
  } else if (a->kind == ValueKindList &&
             b->kind == ValueKindList) {
    ListNode *new_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    new_list->next = list_clone(a->as.list->next, vm_get_frame(vm), vm->current_frame_index);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node)
      node->next = list_clone(b->as.list->next, vm_get_frame(vm), vm->current_frame_index);

    return value_list(new_list, vm_get_frame(vm), vm->current_frame_index);
  } else if (a->kind == ValueKindList) {
    ListNode *new_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    new_list->next = list_clone(a->as.list->next, vm_get_frame(vm), vm->current_frame_index);
    ListNode *node = new_list;

    while (node && node->next)
      node = node->next;

    node->next = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    node->next->value = b;
    node->next->next = NULL;

    return value_list(new_list, vm_get_frame(vm), vm->current_frame_index);
  } else if (b->kind == ValueKindList) {
    ListNode *new_list = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    new_list->next = list_clone(b->as.list->next, vm_get_frame(vm), vm->current_frame_index);

    new_list->next = arena_alloc(&vm_get_frame(vm)->arena, sizeof(ListNode));
    new_list->next->value = a;
    new_list->next->next = list_clone(b->as.list->next, vm_get_frame(vm), vm->current_frame_index);

    return value_list(new_list, vm_get_frame(vm), vm->current_frame_index);
  }

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *sub_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_int(a->as._int - b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindFloat)
    return value_float(a->as._float - b->as._float, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *mul_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt) {
    return value_int(a->as._int * b->as._int, vm_get_frame(vm), vm->current_frame_index);
  } else if (a->kind == ValueKindFloat) {
    return value_float(a->as._float * b->as._float, vm_get_frame(vm), vm->current_frame_index);
  } else if (a->kind == ValueKindString) {
    StringBuilder sb = {0};
    for (u32 i = 0; i < (u32) b->as._int; ++i)
      sb_push_str(&sb, a->as.string);

    Str result = {
      arena_alloc(&vm_get_frame(vm)->arena, sb.len),
      sb.len,
    };
    memcpy(result.ptr, sb.buffer, result.len);

    free(sb.buffer);

    return value_string(result, vm_get_frame(vm), vm->current_frame_index);
  }

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *div_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_int(a->as._int / b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindFloat)
    return value_float(a->as._float / b->as._float, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *mod_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  return value_int(a->as._int % b->as._int, vm_get_frame(vm), vm->current_frame_index);
}

Value *eq_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  return value_bool(value_eq(a, b), vm_get_frame(vm), vm->current_frame_index);
}

Value *ne_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  return value_bool(!value_eq(a, b), vm_get_frame(vm), vm->current_frame_index);
}

Value *ls_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int < b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else
    return value_bool(a->as._float < b->as._float, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *le_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int <= b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindFloat)
    return value_bool(a->as._float <= b->as._float, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *gt_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int > b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindFloat)
    return value_bool(a->as._float > b->as._float, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *ge_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int >= b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindFloat)
    return value_bool(a->as._float >= b->as._float, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *and_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    return value_bool(a->as._int & b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindBool)
    return value_bool(a->as._bool & b->as._bool, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *or_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int | b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindBool)
    return value_bool(a->as._bool | b->as._bool, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *xor_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    return value_bool(a->as._int ^ b->as._int, vm_get_frame(vm), vm->current_frame_index);
  else if (a->kind == ValueKindBool)
    return value_bool(a->as._bool ^ b->as._bool, vm_get_frame(vm), vm->current_frame_index);

  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Value *not_intrinsic(Vm *vm, Value **args) {
  return value_bool(!value_to_bool(args[0]), vm_get_frame(vm), vm->current_frame_index);
}

Value *type_intrinsic(Vm *vm, Value **args) {
  switch (args[0]->kind) {
  case ValueKindUnit: {
    return value_string(STR_LIT("unit"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindList: {
    return value_string(STR_LIT("list"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindString: {
    return value_string(STR_LIT("string"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindInt: {
    return value_string(STR_LIT("int"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindFloat: {
    return value_string(STR_LIT("float"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindBool: {
    return value_string(STR_LIT("bool"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindFunc: {
    return value_string(STR_LIT("func"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindDict: {
    return value_string(STR_LIT("dict"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  case ValueKindEnv: {
    return value_string(STR_LIT("env"), vm_get_frame(vm), vm->current_frame_index);
  } break;

  default: {
    PANIC(vm_get_frame(vm), vm->current_frame_index, "Unknown type: %u\n", args[0]->kind);
  }
  }
}

Value *is_unit_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindUnit, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_list_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindList, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_string_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindString, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_int_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindInt, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_float_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindFloat, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_bool_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindBool, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_func_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindFunc, vm_get_frame(vm), vm->current_frame_index);
}

Value *is_dict_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindDict, vm_get_frame(vm), vm->current_frame_index);
}

Value *make_env_intrinsic(Vm *vm, Value **args) {
  Value *cmd_args = args[0];

  ListNode *node = cmd_args->as.list->next;
  while (node) {
    if (node->value->kind != ValueKindString)
      PANIC(vm_get_frame(vm), vm->current_frame_index,
            "make-env: every program argument should be of type string\n");

    node = node->next;
  }

  Intrinsics intrinsics = {0};
  Vm new_vm = {0};
  DA_APPEND(new_vm.frames, (StackFrame) {0});
  vm_init(&new_vm, cmd_args->as.list, &intrinsics);

  return value_env(new_vm, vm_get_frame(vm), vm->current_frame_index);
}

Value *compile_intrinsic(Vm *vm, Value **args) {
  Value *env = args[0];
  Value *code = args[1];
  Value *path = args[2];

  char *path_cstr = malloc(path->as.string.len + 1);
  memcpy(path_cstr, path->as.string.ptr,
         path->as.string.len);
  path_cstr[path->as.string.len] = '\0';

  Arena ir_arena = {0};
  FilePaths included_files = {0};
  Ir ir = parse_ex(code->as.string, path_cstr, &env->as.env->macros,
                   &included_files, &ir_arena, &env->as.env->arena);
  expand_macros_block(&ir, &env->as.env->macros, NULL, NULL, false, &env->as.env->arena);

  Str bytecode = {0};
  bytecode.ptr = (char *) serialize(&ir, &bytecode.len);
  char *new_ptr = arena_alloc(&vm_get_frame(vm)->arena, bytecode.len);
  memcpy(new_ptr, bytecode.ptr, bytecode.len);
  free(bytecode.ptr);
  bytecode.ptr = new_ptr;

  free(path_cstr);
  arena_free(&ir_arena);
  free(included_files.items);

  return value_string(bytecode, vm_get_frame(vm), vm->current_frame_index);
}

Value *eval_compiled_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  Value *env = args[0];
  Value *bytecode = args[1];

  Arena ir_arena = {0};
  Ir ir = deserialize((u8 *) bytecode->as.string.ptr,
                      bytecode->as.string.len,
                      &ir_arena, &env->as.env->arena);
  Value *result = execute_block(&env->as.env->vm, &ir, true);

  arena_free(&ir_arena);

  return value_clone(result, vm_get_frame(vm), vm->current_frame_index);
}

Value *eval_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  Value *env = args[0];
  Value *code = args[1];
  Value *path = args[2];

  char *path_cstr = malloc(path->as.string.len + 1);
  memcpy(path_cstr, path->as.string.ptr,
         path->as.string.len);
  path_cstr[path->as.string.len] = '\0';

  Arena ir_arena = {0};
  Ir ir = parse_ex(code->as.string, path_cstr, &env->as.env->macros,
                   &env->as.env->included_files, &ir_arena, &env->as.env->arena);
  expand_macros_block(&ir, &env->as.env->macros, NULL, NULL, false, &env->as.env->arena);
  Value *result = execute_block(&env->as.env->vm, &ir, true);

  free(path_cstr);
  arena_free(&ir_arena);

  return value_clone(result, vm_get_frame(vm), vm->current_frame_index);
}

Value *exit_intrinsic(Vm *vm, Value **args) {
  Value *exit_code = args[0];

  vm->exit_code = exit_code->as._int;
  vm->state = ExecStateExit;
  return value_unit(vm_get_frame(vm), vm->current_frame_index);
}

Intrinsic core_intrinsics[] = {
  // Base
  { STR_LIT("head"), true, 1, { ValueKindList }, &head_intrinsic },
  { STR_LIT("tail"), true, 1, { ValueKindList }, &tail_intrinsic },
  { STR_LIT("last"), true, 1, { ValueKindList }, &last_intrinsic },
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
  { STR_LIT("eq"), true, 2, { ValueKindUnit, ValueKindUnit }, &eq_intrinsic },
  { STR_LIT("ne"), true, 2, { ValueKindUnit, ValueKindUnit }, &ne_intrinsic },
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
  { STR_LIT("make-env"), true, 1, { ValueKindList }, &make_env_intrinsic },
  { STR_LIT("compile"), true, 3,
    { ValueKindEnv, ValueKindString, ValueKindString },
    &compile_intrinsic },
  { STR_LIT("eval-compiled"), true, 2,
    { ValueKindEnv, ValueKindString },
    &eval_compiled_intrinsic },
  { STR_LIT("eval"), true, 3,
    { ValueKindEnv, ValueKindString, ValueKindString },
    &eval_intrinsic },
  // Other
  { STR_LIT("exit"), false, 1, { ValueKindInt }, &exit_intrinsic },
};

u32 core_intrinsics_len = ARRAY_LEN(core_intrinsics);
