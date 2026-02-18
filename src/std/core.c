#include "aether/vm.h"
#include "aether/parser.h"
#include "aether/macros.h"
#include "aether/serializer.h"
#include "aether/deserializer.h"
#include "aether/misc.h"
#include "aether/arena.h"
#include "lexgen/runtime.h"

Value *head_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (!value->as.list->next)
    return value_unit(vm->current_frame);

  return value->as.list->next->value;
}

Value *tail_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (!value->as.list->next)
    return value_unit(vm->current_frame);

  ListNode *new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
  new_list->next = value->as.list->next->next;

  return value_list(new_list, vm->current_frame);
}

Value *last_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (!value->as.list->next)
    return value_unit(vm->current_frame);

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
        return value_int(i, vm->current_frame);

      node = node->next;
      ++i;
    }
  } else if (collection->kind == ValueKindString) {
    if (item->as.string.str.len <= collection->as.string.str.len) {
      for (u32 i = 0; i < collection->as.string.str.len - item->as.string.str.len; ++i)
        if (str_eq(STR(collection->as.string.str.ptr + i, item->as.string.str.len),
                   item->as.string.str))
          return value_int(i, vm->current_frame);
    }
  } else if (collection->kind == ValueKindBytes) {
    Str item_string = {
      (char *) item->as.bytes.ptr,
      item->as.bytes.len,
    };

    if (item->as.bytes.len <= collection->as.bytes.len) {
      for (u32 i = 0; i < collection->as.bytes.len - item->as.bytes.len; ++i)
        if (str_eq(STR((char *) collection->as.bytes.ptr + i, item->as.bytes.len),
                   item_string))
          return value_int(i, vm->current_frame);
    }
  }

  return value_unit(vm->current_frame);
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

    return value_int(len, vm->current_frame);
  } else if (value->kind == ValueKindString) {
    u32 len = 0;
    u32 index = 0;
    u32 wchar_len;

    while (get_next_wchar(value->as.string.str, index, &wchar_len)) {
      ++len;
      index += wchar_len;
    }

    return value_int(len, vm->current_frame);
  } else if (value->kind == ValueKindBytes) {
    return value_int(value->as.bytes.len, vm->current_frame);
  }

  return value_unit(vm->current_frame);
}

Value *len_bytes_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindString)
    return value_int(value->as.string.str.len, vm->current_frame);
  else if (value->kind == ValueKindBytes)
    return value_int(value->as.bytes.len, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *get_range_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];
  Value *begin = args[1];
  Value *end = args[2];

  Value *len;
  if (value->kind == ValueKindBytes)
    len = len_bytes_intrinsic(vm, &value);
  else
    len = len_intrinsic(vm, &value);

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
    ListNode *sub_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < (u32) begin->as._int; ++i)
      node = node->next;

    for (u32 i = 0; i < (u32) end->as._int - begin->as._int; ++i) {
      sub_list_node->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    sub_list_node->next = NULL;

    return value_list(sub_list, vm->current_frame);
  } else if (value->kind == ValueKindString) {
    u32 begin_byte = 0;
    u32 current_index = 0;
    u32 bytes_len = 0;
    u32 wchar_len;

    if (begin->as._int == end->as._int)
      return value_string((Str) {0}, vm->current_frame);

    while (get_next_wchar(value->as.string.str, current_index, &wchar_len) != '\0' &&
           current_index < end->as._int) {
      if (current_index == begin->as._int)
        begin_byte = bytes_len;

      ++current_index;
      bytes_len += wchar_len;
    }

    Str sub_string = {
      value->as.string.str.ptr + begin_byte,
      bytes_len - begin_byte,
    };

    return value_string(sub_string, vm->current_frame);
  } else if (value->kind == ValueKindBytes) {
    Bytes sub_bytes = {
      value->as.bytes.ptr + begin->as._int,
      end->as._int - begin->as._int,
    };

    return value_bytes(sub_bytes, vm->current_frame);
  }

  return value_unit(vm->current_frame);
}

Value *gen_range_intrinsic(Vm *vm, Value **args) {
  Value *begin = args[0];
  Value *end = args[1];

  ListNode *range = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));

  ListNode **next = &range->next;
  for (i64 i = begin->as._int; i < end->as._int; ++i) {
    (*next) = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    (*next)->value = value_alloc(vm->current_frame);
    (*next)->value->kind = ValueKindInt;
    (*next)->value->as._int = i;

    next = &(*next)->next;
  }

  *next = NULL;

  return value_list(range, vm->current_frame);
}

Value *map_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *list = args[1];

  ListNode *new_list = NULL;
  ListNode **new_list_next = NULL;
  if (!list->is_atom) {
    new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    new_list_next = &new_list->next;
  }

  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);

    // TODO: put real metadata here
    execute_func(vm, func->as.func, NULL);

    if (vm->state == ExecStateExit)
      break;

    Value *replacement = stack_last(vm);

    if (list->is_atom) {
      --node->value->refs_count;
      node->value = replacement;
    } else {
      *new_list_next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      (*new_list_next)->value = replacement;
      new_list_next = &(*new_list_next)->next;
    }

    *new_list_next = NULL;

    vm->stack.len -= 2;
    node = node->next;
  }

  if (list->is_atom)
    return list;
  return value_list(new_list, vm->current_frame);
}

Value *filter_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *list = args[1];

  ListNode *new_list = NULL;
  ListNode **new_list_next = NULL;
  if (!list->is_atom) {
    new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    new_list_next = &new_list->next;
  }

  ListNode *prev_node = list->as.list;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, node->value);

    // TODO: put real metadata here
    execute_func(vm, func->as.func, NULL);

    if (vm->state == ExecStateExit)
      break;

    Value *is_ok = stack_last(vm);

    if (is_ok->kind != ValueKindBool) {
      ERROR("filter: predicate should return bool\n");
      vm->state = ExecStateExit;
      vm->exit_code = 1;
      return value_unit(vm->current_frame);
    }

    if (is_ok->as._bool) {
      if (!list->is_atom) {
        *new_list_next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
        (*new_list_next)->value = node->value;
        new_list_next = &(*new_list_next)->next;
      }

      prev_node = prev_node->next;
    } else if (list->is_atom) {
      --prev_node->next->value->refs_count;
      prev_node->next = node->next;
    }

    vm->stack.len -= 2;
    node = node->next;
  }

  *new_list_next = NULL;

  if (list->is_atom)
    return list;
  return value_list(new_list, vm->current_frame);
}

Value *fold_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *initial_value = args[1];
  Value *list = args[2];

  Value *accumulator = initial_value;
  ListNode *node = list->as.list->next;
  while (node) {
    DA_APPEND(vm->stack, accumulator);
    DA_APPEND(vm->stack, node->value);

    // TODO: put real metadata here
    execute_func(vm, func->as.func, NULL);

    if (vm->state == ExecStateExit)
      break;

    accumulator = stack_last(vm);

    vm->stack.len -= 3;
    node = node->next;
  }

  return accumulator;
}

Value *zip_intrinsic(Vm *vm,Value **args) {
  Value *list_a = args[0];
  Value *list_b = args[1];

  ListNode *new_list = NULL;
  ListNode **new_list_next = NULL;
  if (!list_a->is_atom) {
    new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    new_list_next = &new_list->next;
  }

  ListNode *node_a = list_a->as.list->next;
  ListNode *node_b = list_b->as.list->next;
  while (node_a && node_b) {
    *new_list_next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));

    ListNode *pair = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    pair->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    pair->next->value = node_a->value;
    pair->next->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    pair->next->next->value = node_b->value;

    if (list_a->is_atom) {
      node_a->value = value_list(pair, vm->current_frame);
    } else {
      (*new_list_next)->value = value_list(pair, vm->current_frame);
      new_list_next = &(*new_list_next)->next;
    }

    node_a = node_a->next;
    node_b = node_b->next;
  }

  *new_list_next = NULL;

  return value_list(new_list, vm->current_frame);
}

// Shellsort with Ciura gap sequence
Value *sort_intrinsic(Vm *vm, Value **args) {
  Value *func = args[0];
  Value *list = args[1];

  Value *len = len_intrinsic(vm, args + 1);
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

      while (k >= gaps[i]) {
        DA_APPEND(vm->stack, sorted[k - gaps[i]]);
        DA_APPEND(vm->stack, temp);
        execute_func(vm, func->as.func, NULL);

        if (!value_to_bool(stack_last(vm))) {
          vm->stack.len -= 3;
          break;
        }
        vm->stack.len -= 3;
        sorted[k] = sorted[k - gaps[i]];
        k -= gaps[i];
      }

      sorted[k] = temp;
    }
  }

  if (list->is_atom) {
    ListNode *node = list->as.list->next;
    for (u32 i = 0; i < (u32) len->as._int; ++i) {
      node->value = sorted[i];
      node = node->next;
    }

    free(sorted);

    return list;
  }

  ListNode *result = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));

  ListNode **next = &result->next;
  for (u32 i = 0; i < (u32) len->as._int; ++i) {
    *next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    (*next)->value = value_clone(sorted[i], vm->current_frame);
    next = &(*next)->next;
  }

  *next = NULL;

  free(sorted);

  return value_list(result, vm->current_frame);
}

Value *for_each_intrinsic(Vm *vm, Value **args) {
  Value *collection = args[0];
  Value *func = args[1];

  if (collection->kind == ValueKindList) {
    ListNode *node = collection->as.list->next;
    while (node) {
      DA_APPEND(vm->stack, node->value);

      execute_func(vm, func->as.func, NULL);

      if (vm->state != ExecStateContinue) {
        Value *result = stack_last(vm);

        if (vm->state == ExecStateReturn &&
            result->kind == ValueKindUnit)
          vm->state = ExecStateContinue;

        --vm->stack.len;
        return result;
      }

      --vm->stack.len;

      node = node->next;
    }
  } else if (collection->kind == ValueKindString) {
    char *filler = arena_alloc(&vm->current_frame->arena, sizeof(wchar));
    Value *_char = value_string(STR(filler, sizeof(wchar)), vm->current_frame);
    DA_APPEND(vm->stack, _char);

    u32 wchar_len;
    u32 index = 0;
    wchar _wchar;

    while ((_wchar = get_next_wchar(collection->as.string.str, index, &wchar_len)) != '\0') {
      *(wchar *) _char->as.string.str.ptr = _wchar;

      execute_func(vm, func->as.func, NULL);

      if (vm->state != ExecStateContinue) {
        Value *result = stack_last(vm);

        if (vm->state == ExecStateReturn &&
            result->kind == ValueKindUnit)
          vm->state = ExecStateContinue;

        --vm->stack.len;
        return result;
      }

      index += wchar_len;
      --vm->stack.len;
    }

    --vm->stack.len;
  } else if (collection->kind == ValueKindBytes) {
    Value *_int = value_int(0, vm->current_frame);
    DA_APPEND(vm->stack, _int);

    for (u32 i = 0; i < collection->as.bytes.len; ++i) {
      _int->as._int = collection->as.bytes.ptr[i];

      execute_func(vm, func->as.func, NULL);

      if (vm->state != ExecStateContinue) {
        Value *result = stack_last(vm);

        if (vm->state == ExecStateReturn &&
            result->kind == ValueKindUnit)
          vm->state = ExecStateContinue;

        --vm->stack.len;
        return result;
      }

      --vm->stack.len;
    }

    --vm->stack.len;
  } else if (collection->kind == ValueKindDict) {
    Dict *_pair = arena_alloc(&vm->current_frame->arena, sizeof(Dict));
    memset(_pair, 0, sizeof(Dict));

    Value *key = value_string(STR_LIT("key"), vm->current_frame);
    Value *value = value_string(STR_LIT("value"), vm->current_frame);

    Value *pair = value_dict(_pair, vm->current_frame);
    DA_APPEND(vm->stack, pair);

    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = collection->as.dict->items[i];
      while (entry) {
        dict_set_value(vm->current_frame, _pair, key, entry->key);
        dict_set_value(vm->current_frame, _pair, value, entry->value);

        execute_func(vm, func->as.func, NULL);

        if (vm->state != ExecStateContinue) {
          Value *result = stack_last(vm);

          if (vm->state == ExecStateReturn &&
            result->kind == ValueKindUnit)
          vm->state = ExecStateContinue;

          --vm->stack.len;
          return result;
        }

        --vm->stack.len;

        entry = entry->next;
      }
    }

    *pair->as.dict = (Dict) {0};
    --vm->stack.len;
  }

  return value_unit(vm->current_frame);
}

Value *to_str_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  Str string = value_to_str(value, false, false, &vm->current_frame->arena);

  return value_string(string, vm->current_frame);
}

Value *to_bytes_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  Bytes bytes = {0};

  if (value->kind == ValueKindString) {
    bytes.len = value->as.string.str.len;
    bytes.ptr = arena_alloc(&vm->current_frame->arena, bytes.len);
    memcpy(bytes.ptr, value->as.string.str.ptr, bytes.len);
  } else if (value->kind == ValueKindInt) {
    bytes.len = sizeof(value->as._int);
    bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(value->as._int));
    *bytes.ptr = value->as._int;
  } else if (value->kind == ValueKindFloat) {
    bytes.len = sizeof(value->as._float);
    bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(value->as._float));
    *bytes.ptr = value->as._float;
  } else if (value->kind == ValueKindBool) {
    bytes.len = sizeof(value->as._bool);
    bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(value->as._bool));
    *bytes.ptr = value->as._bool;
  }

  if (bytes.len == 0)
    return value_unit(vm->current_frame);

  return value_bytes(bytes, vm->current_frame);
}

Value *to_byte8_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  Bytes bytes = {0};
  bytes.len = sizeof(u8);
  bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(u8));
  *bytes.ptr = value->as._int;

  return value_bytes(bytes, vm->current_frame);
}

Value *to_byte16_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  Bytes bytes = {0};
  bytes.len = sizeof(u16);
  bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(u16));
  *(u16 *) bytes.ptr = value->as._int;

  return value_bytes(bytes, vm->current_frame);
}

Value *to_byte32_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  Bytes bytes = {0};
  bytes.len = sizeof(u32);
  bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(u32));
  *(u32 *) bytes.ptr = value->as._int;

  return value_bytes(bytes, vm->current_frame);
}

Value *to_byte64_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  Bytes bytes = {0};
  bytes.len = sizeof(u64);
  bytes.ptr = arena_alloc(&vm->current_frame->arena, sizeof(u64));
  *(u64 *) bytes.ptr = value->as._int;

  return value_bytes(bytes, vm->current_frame);
}

Value *to_int_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindString)
    return value_int(str_to_i64(value->as.string.str), vm->current_frame);
  else if (value->kind == ValueKindBool)
    return value_int((i64) value->as._bool, vm->current_frame);
  else if (value->kind == ValueKindFloat)
    return value_int((i64) value->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *to_int8_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->as.bytes.len >= sizeof(u8))
    return value_int(value->as.bytes.ptr[0], vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *to_int16_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->as.bytes.len >= sizeof(u16))
    return value_int(((u16 *) value->as.bytes.ptr)[0], vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *to_int32_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->as.bytes.len >= sizeof(u32))
    return value_int(((u32 *) value->as.bytes.ptr)[0], vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *to_int64_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->as.bytes.len >= sizeof(u64))
    return value_int(((u64 *) value->as.bytes.ptr)[0], vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *to_float_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  if (value->kind == ValueKindInt) {
    return value_float((f64) value->as._int, vm->current_frame);
  } else if (value->kind == ValueKindString) {
    return value_float(str_to_f64(value->as.string.str), vm->current_frame);
  } else if (value->kind == ValueKindBytes) {
    if (value->as.bytes.len >= sizeof(value->as._float))
      return value_float(*(f64 *) value->as.bytes.ptr, vm->current_frame);
  }

  return value_unit(vm->current_frame);
}

Value *to_bool_intrinsic(Vm *vm, Value **args) {
  Value *value = args[0];

  return value_bool(value_to_bool(value), vm->current_frame);
}

Value *add_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt) {
    return value_int(a->as._int + b->as._int, vm->current_frame);
  } else if (a->kind == ValueKindFloat &&
             b->kind == ValueKindFloat) {
    return value_float(a->as._float + b->as._float, vm->current_frame);
  } else if (a->kind == ValueKindString &&
             b->kind == ValueKindString) {
    Str new_string;
    new_string.len = a->as.string.str.len + b->as.string.str.len;
    new_string.ptr = arena_alloc(&vm->current_frame->arena, new_string.len);
    memcpy(new_string.ptr, a->as.string.str.ptr, a->as.string.str.len);
    memcpy(new_string.ptr + a->as.string.str.len,
           b->as.string.str.ptr, b->as.string.str.len);

    if (a->is_atom) {
      a->as.string.str = new_string;
      return a;
    }
    return value_string(new_string, vm->current_frame);
  } else if (a->kind == ValueKindList &&
             b->kind == ValueKindList) {
    ListNode *new_list = a->as.list;
    if (!a->is_atom) {
      new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      new_list->next = list_clone(a->as.list->next, vm->current_frame);
    }

    ListNode *last = new_list;
    while (last && last->next)
      last = last->next;

    if (a->is_atom)
      last->next = list_clone(b->as.list->next, a->frame);
    else
      last->next = list_clone(b->as.list->next, vm->current_frame);

    if (a->is_atom)
      return a;
    return value_list(new_list, vm->current_frame);
  } else if (a->kind == ValueKindBytes &&
             b->kind == ValueKindBytes) {
    Bytes bytes;
    bytes.len = a->as.bytes.len + b->as.bytes.len;
    bytes.ptr = arena_alloc(&vm->current_frame->arena, bytes.len);
    memcpy(bytes.ptr, a->as.bytes.ptr, a->as.bytes.len);
    memcpy(bytes.ptr + a->as.bytes.len, b->as.bytes.ptr, b->as.bytes.len);

    if (a->is_atom) {
      a->as.bytes = bytes;
      return a;
    }
    return value_bytes(bytes, vm->current_frame);
  } else if (a->kind == ValueKindDict) {
    Value *dict_value = a;

    if (!a->is_atom)
      dict_value = value_clone(dict_value, vm->current_frame);

    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = b->as.dict->items[i];
      while (entry) {
        dict_set_value(vm->current_frame, dict_value->as.dict,
                       entry->key, entry->value);

        entry = entry->next;
      }
    }

    return dict_value;
  }

  return value_unit(vm->current_frame);
}

Value *sub_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_int(a->as._int - b->as._int, vm->current_frame);
  else if (a->kind == ValueKindFloat)
    return value_float(a->as._float - b->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *mul_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt) {
    return value_int(a->as._int * b->as._int, vm->current_frame);
  } else if (a->kind == ValueKindFloat) {
    return value_float(a->as._float * b->as._float, vm->current_frame);
  } else if (a->kind == ValueKindString) {
    StringBuilder sb = {0};
    for (u32 i = 0; i < (u32) b->as._int; ++i)
      sb_push_str(&sb, a->as.string.str);

    Str result = {
      arena_alloc(&vm->current_frame->arena, sb.len),
      sb.len,
    };
    memcpy(result.ptr, sb.buffer, result.len);

    free(sb.buffer);

    return value_string(result, vm->current_frame);
  }

  return value_unit(vm->current_frame);
}

Value *div_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_int(a->as._int / b->as._int, vm->current_frame);
  else if (a->kind == ValueKindFloat)
    return value_float(a->as._float / b->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *mod_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  return value_int(a->as._int % b->as._int, vm->current_frame);
}

Value *before_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  ListNode *new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
  new_list->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
  new_list->next->value = a;
  new_list->next->next = list_clone(b->as.list->next, vm->current_frame);

  return value_list(new_list, vm->current_frame);
}

Value *after_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  ListNode *new_list = a->as.list;
  if (!a->is_atom) {
    new_list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
    new_list->next = list_clone(a->as.list->next, vm->current_frame);
  }

  ListNode *last = new_list;
  while (last && last->next)
    last = last->next;

  if (a->is_atom)
    last->next = arena_alloc(&a->frame->arena, sizeof(ListNode));
  else
    last->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));

  if (a->is_atom && b->frame != a->frame)
    last->next->value = value_clone(b, a->frame);
  else
    last->next->value = b;

  last->next->next = NULL;

  if (a->is_atom)
    return a;
  return value_list(new_list, vm->current_frame);
}

Value *eq_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  return value_bool(value_eq(a, b), vm->current_frame);
}

Value *ne_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  return value_bool(!value_eq(a, b), vm->current_frame);
}

Value *ls_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int < b->as._int, vm->current_frame);
  else
    return value_bool(a->as._float < b->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *le_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int <= b->as._int, vm->current_frame);
  else if (a->kind == ValueKindFloat)
    return value_bool(a->as._float <= b->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *gt_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int > b->as._int, vm->current_frame);
  else if (a->kind == ValueKindFloat)
    return value_bool(a->as._float > b->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *ge_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int >= b->as._int, vm->current_frame);
  else if (a->kind == ValueKindFloat)
    return value_bool(a->as._float >= b->as._float, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *and_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    return value_bool(a->as._int & b->as._int, vm->current_frame);
  else if (a->kind == ValueKindBool)
    return value_bool(a->as._bool & b->as._bool, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *or_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt)
    return value_bool(a->as._int | b->as._int, vm->current_frame);
  else if (a->kind == ValueKindBool)
    return value_bool(a->as._bool | b->as._bool, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *xor_intrinsic(Vm *vm, Value **args) {
  Value *a = args[0];
  Value *b = args[1];

  if (a->kind == ValueKindInt &&
      b->kind == ValueKindInt)
    return value_bool(a->as._int ^ b->as._int, vm->current_frame);
  else if (a->kind == ValueKindBool)
    return value_bool(a->as._bool ^ b->as._bool, vm->current_frame);

  return value_unit(vm->current_frame);
}

Value *not_intrinsic(Vm *vm, Value **args) {
  return value_bool(!value_to_bool(args[0]), vm->current_frame);
}

Value *type_intrinsic(Vm *vm, Value **args) {
  switch (args[0]->kind) {
  case ValueKindUnit: {
    return value_string(STR_LIT("unit"), vm->current_frame);
  } break;

  case ValueKindList: {
    return value_string(STR_LIT("list"), vm->current_frame);
  } break;

  case ValueKindString: {
    return value_string(STR_LIT("string"), vm->current_frame);
  } break;

  case ValueKindInt: {
    return value_string(STR_LIT("int"), vm->current_frame);
  } break;

  case ValueKindFloat: {
    return value_string(STR_LIT("float"), vm->current_frame);
  } break;

  case ValueKindBool: {
    return value_string(STR_LIT("bool"), vm->current_frame);
  } break;

  case ValueKindFunc: {
    return value_string(STR_LIT("func"), vm->current_frame);
  } break;

  case ValueKindDict: {
    return value_string(STR_LIT("dict"), vm->current_frame);
  } break;

  case ValueKindEnv: {
    return value_string(STR_LIT("env"), vm->current_frame);
  } break;

  case ValueKindBytes: {
    return value_string(STR_LIT("bytes"), vm->current_frame);
  } break;

  default: {
    ERROR("Unknown value king: %u\n", args[0]->kind);
    vm->state = ExecStateExit;
    vm->exit_code = 1;
    return value_unit(vm->current_frame);
  }
  }
}

Value *is_unit_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindUnit, vm->current_frame);
}

Value *is_list_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindList, vm->current_frame);
}

Value *is_string_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindString, vm->current_frame);
}

Value *is_int_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindInt, vm->current_frame);
}

Value *is_float_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindFloat, vm->current_frame);
}

Value *is_bool_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindBool, vm->current_frame);
}

Value *is_func_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindFunc, vm->current_frame);
}

Value *is_dict_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindDict, vm->current_frame);
}

Value *is_env_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindEnv, vm->current_frame);
}

Value *is_bytes_intrinsic(Vm *vm, Value **args) {
  return value_bool(args[0]->kind == ValueKindBytes, vm->current_frame);
}

static char *str_to_cstr(Str str) {
  StringBuilder sb = {0};

  sb_push_str(&sb, str);
  sb_push_char(&sb, '\0');

  return sb.buffer;
}

Value *make_env_intrinsic(Vm *vm, Value **args) {
  Value *cmd_args = args[0];

  Da(char *) cstr_cmd_args = {0};

  ListNode *node = cmd_args->as.list->next;
  while (node) {
    if (node->value->kind != ValueKindString) {
      ERROR("make-env: every program argument should be of type string\n");
      vm->state = ExecStateExit;
      vm->exit_code = 1;
      return value_unit(vm->current_frame);
    }

    char *cstr_cmd_arg = str_to_cstr(node->value->as.string.str);
    DA_APPEND(cstr_cmd_args, cstr_cmd_arg);

    node = node->next;
  }

  Intrinsics intrinsics = {0};
  Vm *new_vm = arena_alloc(&vm->current_frame->arena, sizeof(Vm));
  *new_vm = vm_create(cstr_cmd_args.len, cstr_cmd_args.items, &intrinsics);

  for (u32 i = 0; i < cstr_cmd_args.len; ++i)
    free(cstr_cmd_args.items[i]);
  free(cstr_cmd_args.items);

  return value_env(new_vm, vm->current_frame);
}

static Value *process_include_paths(IncludePaths *dest, Value *value, char *func_name, Vm *vm) {
  ListNode *include_path = value->as.list->next;
  while (include_path) {
    if (include_path->value->kind != ValueKindString) {
      ERROR("%s: every include path should be of type string\n", func_name);
      vm->state = ExecStateExit;
      vm->exit_code = 1;
      return value_unit(vm->current_frame);
    }

    DA_APPEND(*dest, include_path->value->as.string.str);

    include_path = include_path->next;
  }

  return NULL;
}

Value *compile_intrinsic(Vm *vm, Value **args) {
  Value *env = args[0];
  Value *code = args[1];
  Value *path = args[2];
  Value *include_paths = args[3];
  Value *compile_macros = args[4];

  Str magic = {
    code->as.string.str.ptr,
    sizeof(u32),
  };

  if (str_eq(magic, STR_LIT("ABC\0")) ||
      str_eq(magic, STR_LIT("ABM\0")))
    return code;

  u32 prev_macros_len = env->as.env->macros.len;

  Arena ir_arena = {0};
  FilePaths included_files = {0};
  IncludePaths _include_paths = {0};
  CachedASTs cached_asts = {0};

  Str file_dir = get_file_dir(path->as.string.str);
  DA_APPEND(_include_paths, file_dir);

  Value *include_result = process_include_paths(&_include_paths, include_paths, "compile", vm);
  if (include_result)
    return include_result;

  Exprs ast = parse_ex(code->as.string.str, &path->as.string.str,
                       &env->as.env->macros, &included_files, &_include_paths,
                       &cached_asts, &ir_arena, true, compile_macros->as._bool, NULL);

  expand_macros_block(&ast, &env->as.env->macros, NULL, NULL, false,
                      &ir_arena, &path->as.string.str, 0, 0, false);

  Ir ir = ast_to_ir(&ast, &ir_arena);

  Str bytecode = {0};
  bytecode.ptr = (char *) serialize(ir, &bytecode.len, &included_files);

  char *new_ptr = arena_alloc(&vm->current_frame->arena, bytecode.len);
  memcpy(new_ptr, bytecode.ptr, bytecode.len);
  free(bytecode.ptr);
  bytecode.ptr = new_ptr;

  Dict *dict = arena_alloc(&vm->current_frame->arena, sizeof(Dict));
  memset(dict, 0, sizeof(Dict));

  Value *compiled = value_string(bytecode, vm->current_frame);
  Value *compiled_key = value_string(STR_LIT("compiled"), vm->current_frame);
  dict_set_value(vm->current_frame, dict, compiled_key, compiled);

  Value *compiled_macros;

  if (compile_macros->as._bool && env->as.env->macros.len > prev_macros_len) {
    Macros new_macros = {
      env->as.env->macros.items + prev_macros_len,
      env->as.env->macros.len - prev_macros_len,
      env->as.env->macros.len - prev_macros_len,
    };
    Str macros_bytecode = {0};

    macros_bytecode.ptr = (char *) serialize_macros(&new_macros,
                                                    &macros_bytecode.len,
                                                    &included_files);

    char *new_ptr = arena_alloc(&vm->current_frame->arena, macros_bytecode.len);
    memcpy(new_ptr, macros_bytecode.ptr, macros_bytecode.len);
    free(macros_bytecode.ptr);
    macros_bytecode.ptr = new_ptr;

    compiled_macros = value_string(macros_bytecode, vm->current_frame);
  } else {
    compiled_macros = value_unit(vm->current_frame);
  }

  Value *compiled_macros_key = value_string(STR_LIT("compiled-macros"), vm->current_frame);
  dict_set_value(vm->current_frame, dict, compiled_macros_key, compiled_macros);

  if (included_files.items)
    free(included_files.items);

  if (_include_paths.items)
    free(_include_paths.items);

  return value_dict(dict, vm->current_frame);
}

Value *eval_compiled_intrinsic(Vm *vm, Value **args) {
  Value *env = args[0];
  Value *bytecode = args[1];

  Arena ir_arena = {0};
  Ir ir = deserialize(bytecode->as.bytes.ptr,
                      bytecode->as.bytes.len, &ir_arena,
                      &env->as.env->vm->current_file_path);

  DA_APPEND(env->as.env->cached_irs, ir);

  Value *result = execute_get(env->as.env->vm, ir);

  if (env->as.env->vm->state != ExecStateContinue)
    env->as.env->vm->state = ExecStateContinue;

  if (env->as.env->vm->exit_code != 0)
    env->as.env->vm->exit_code = 0;

  return value_clone(result, vm->current_frame);
}

Value *eval_macros_intrinsic(Vm *vm, Value **args) {
  Value *env = args[0];
  Value *macro_bytecode = args[1];

  Arena ir_arena = {0};
  Macros macros = deserialize_macros(macro_bytecode->as.bytes.ptr,
                                     macro_bytecode->as.bytes.len,
                                     &env->as.env->included_files, &ir_arena);

  if (env->as.env->macros.cap < env->as.env->macros.len + macros.len) {
    env->as.env->macros.cap = env->as.env->macros.len + macros.len;
    env->as.env->macros.items = realloc(env->as.env->macros.items,
                                        env->as.env->macros.cap * sizeof(Macro));
  }

  memcpy(env->as.env->macros.items + env->as.env->macros.len,
         macros.items, macros.len * sizeof(Macro));
  env->as.env->macros.len += macros.len;

  free(macros.items);

  return value_unit(vm->current_frame);
}

Value *eval_intrinsic(Vm *vm, Value **args) {
  Value *env = args[0];
  Value *code = args[1];
  Value *path = args[2];
  Value *include_paths = args[3];

  Arena ir_arena = {0};
  FilePaths included_files = {0};
  IncludePaths _include_paths = {0};

  Str file_dir = get_file_dir(path->as.string.str);
  DA_APPEND(_include_paths, file_dir);

  Value *include_result = process_include_paths(&_include_paths, include_paths, "eval", vm);
  if (include_result)
    return include_result;

  Exprs ast = parse_ex(code->as.string.str, &path->as.string.str,
                       &env->as.env->macros, &included_files, &_include_paths,
                       &env->as.env->cached_asts, &ir_arena, false, false, NULL);

  expand_macros_block(&ast, &env->as.env->macros, NULL, NULL, false,
                      &ir_arena, &path->as.string.str, 0, 0, false);

  Ir ir = ast_to_ir(&ast, &ir_arena);

  DA_APPEND(env->as.env->cached_irs, ir);

  env->as.env->vm->current_file_path = path->as.string.str;

  Value *result = execute_get(env->as.env->vm, ir);

  if (included_files.items)
    free(included_files.items);

  if (_include_paths.items)
    free(_include_paths.items);

  if (env->as.env->vm->state != ExecStateContinue)
    env->as.env->vm->state = ExecStateContinue;

  if (env->as.env->vm->exit_code != 0)
    env->as.env->vm->exit_code = 0;

  return value_clone(result, vm->current_frame);
}

Value *atom_intrinsic(Vm *vm, Value **args) {
  (void) vm;

  Value *value = args[0];

  value->is_atom = true;

  return value;
}

Value *copy_intrinsic(Vm *vm, Value **args) {
  return value_clone(args[0], vm->current_frame);
}

Value *exit_intrinsic(Vm *vm, Value **args) {
  Value *exit_code = args[0];

  vm->exit_code = exit_code->as._int;
  vm->state = ExecStateExit;
  return value_unit(vm->current_frame);
}

Intrinsic core_intrinsics[] = {
  // Base
  { STR_LIT("head"), true, 1, { ValueKindList }, &head_intrinsic, NULL },
  { STR_LIT("tail"), true, 1, { ValueKindList }, &tail_intrinsic, NULL },
  { STR_LIT("last"), true, 1, { ValueKindList }, &last_intrinsic, NULL },
  { STR_LIT("get-index"), true, 2, { ValueKindList, ValueKindUnit }, &get_index_intrinsic, NULL },
  { STR_LIT("get-index"), true, 2, { ValueKindString, ValueKindString }, &get_index_intrinsic, NULL },
  { STR_LIT("get-index"), true, 2, { ValueKindBytes, ValueKindBytes }, &get_index_intrinsic, NULL },
  { STR_LIT("len"), true, 1, { ValueKindList }, &len_intrinsic, NULL },
  { STR_LIT("len"), true, 1, { ValueKindString }, &len_intrinsic, NULL },
  { STR_LIT("len"), true, 1, { ValueKindBytes }, &len_intrinsic, NULL },
  { STR_LIT("len-bytes"), true, 1, { ValueKindString }, &len_bytes_intrinsic, NULL },
  { STR_LIT("len-bytes"), true, 1, { ValueKindBytes }, &len_bytes_intrinsic, NULL },
  { STR_LIT("get-range"), true, 3,
    { ValueKindList, ValueKindInt, ValueKindInt },
    &get_range_intrinsic, NULL },
  { STR_LIT("get-range"), true, 3,
    { ValueKindString, ValueKindInt, ValueKindInt },
    &get_range_intrinsic, NULL },
  { STR_LIT("get-range"), true, 3,
    { ValueKindBytes, ValueKindInt, ValueKindInt },
    &get_range_intrinsic, NULL },
  { STR_LIT("gen-range"), true, 2, { ValueKindInt, ValueKindInt }, &gen_range_intrinsic, NULL },
  { STR_LIT("map"), true, 2, { ValueKindFunc, ValueKindList }, &map_intrinsic, NULL },
  { STR_LIT("filter"), true, 2, { ValueKindFunc, ValueKindList }, &filter_intrinsic, NULL },
  { STR_LIT("fold"), true, 3,
    { ValueKindFunc, ValueKindUnit, ValueKindList },
    &fold_intrinsic, NULL },
  { STR_LIT("zip"), true, 2, { ValueKindList, ValueKindList }, &zip_intrinsic, NULL },
  { STR_LIT("sort"), true, 2, { ValueKindFunc, ValueKindList }, &sort_intrinsic, NULL },
  { STR_LIT("for-each"), false, 2, { ValueKindList, ValueKindFunc }, &for_each_intrinsic, NULL },
  { STR_LIT("for-each"), false, 2, { ValueKindString, ValueKindFunc }, &for_each_intrinsic, NULL },
  { STR_LIT("for-each"), false, 2, { ValueKindBytes, ValueKindFunc }, &for_each_intrinsic, NULL },
  { STR_LIT("for-each"), false, 2, { ValueKindDict, ValueKindFunc }, &for_each_intrinsic, NULL },
  // Conversions
  { STR_LIT("to-str"), true, 1, { ValueKindUnit }, &to_str_intrinsic, NULL },
  { STR_LIT("to-bytes"), true, 1, { ValueKindString }, &to_bytes_intrinsic, NULL },
  { STR_LIT("to-bytes"), true, 1, { ValueKindInt }, &to_bytes_intrinsic, NULL },
  { STR_LIT("to-bytes"), true, 1, { ValueKindFloat }, &to_bytes_intrinsic, NULL },
  { STR_LIT("to-bytes"), true, 1, { ValueKindBool }, &to_bytes_intrinsic, NULL },
  { STR_LIT("to-byte8"), true, 1, { ValueKindInt }, &to_byte8_intrinsic, NULL },
  { STR_LIT("to-byte16"), true, 1, { ValueKindInt }, &to_byte16_intrinsic, NULL },
  { STR_LIT("to-byte32"), true, 1, { ValueKindInt }, &to_byte32_intrinsic, NULL },
  { STR_LIT("to-byte64"), true, 1, { ValueKindInt }, &to_byte64_intrinsic, NULL },
  { STR_LIT("to-int"), true, 1, { ValueKindString }, &to_int_intrinsic, NULL },
  { STR_LIT("to-int"), true, 1, { ValueKindBool }, &to_int_intrinsic, NULL },
  { STR_LIT("to-int"), true, 1, { ValueKindFloat }, &to_int_intrinsic, NULL },
  { STR_LIT("to-int8"), true, 1, { ValueKindBytes }, &to_int8_intrinsic, NULL },
  { STR_LIT("to-int16"), true, 1, { ValueKindBytes }, &to_int16_intrinsic, NULL },
  { STR_LIT("to-int32"), true, 1, { ValueKindBytes }, &to_int32_intrinsic, NULL },
  { STR_LIT("to-int64"), true, 1, { ValueKindBytes }, &to_int64_intrinsic, NULL },
  { STR_LIT("to-float"), true, 1, { ValueKindInt }, &to_float_intrinsic, NULL },
  { STR_LIT("to-float"), true, 1, { ValueKindString }, &to_float_intrinsic, NULL },
  { STR_LIT("to-float"), true, 1, { ValueKindBytes }, &to_float_intrinsic, NULL },
  { STR_LIT("to-bool"), true, 1, { ValueKindUnit }, &to_bool_intrinsic, NULL },
  // Math
  { STR_LIT("add"),    true, 2, { ValueKindInt, ValueKindInt }, &add_intrinsic, NULL },
  { STR_LIT("add"),    true, 2, { ValueKindFloat, ValueKindFloat }, &add_intrinsic, NULL },
  { STR_LIT("add"),    true, 2, { ValueKindString, ValueKindString }, &add_intrinsic, NULL },
  { STR_LIT("add"),    true, 2, { ValueKindList, ValueKindList }, &add_intrinsic, NULL },
  { STR_LIT("add"),    true, 2, { ValueKindBytes, ValueKindBytes }, &add_intrinsic, NULL },
  { STR_LIT("add"),    true, 2, { ValueKindDict, ValueKindDict }, &add_intrinsic, NULL },
  { STR_LIT("sub"),    true, 2, { ValueKindInt, ValueKindInt }, &sub_intrinsic, NULL },
  { STR_LIT("sub"),    true, 2, { ValueKindFloat, ValueKindFloat }, &sub_intrinsic, NULL },
  { STR_LIT("mul"),    true, 2, { ValueKindInt, ValueKindInt }, &mul_intrinsic, NULL },
  { STR_LIT("mul"),    true, 2, { ValueKindFloat, ValueKindFloat }, &mul_intrinsic, NULL },
  { STR_LIT("mul"),    true, 2, { ValueKindString, ValueKindInt }, &mul_intrinsic, NULL },
  { STR_LIT("div"),    true, 2, { ValueKindInt, ValueKindInt }, &div_intrinsic, NULL },
  { STR_LIT("div"),    true, 2, { ValueKindFloat, ValueKindFloat }, &div_intrinsic, NULL },
  { STR_LIT("mod"),    true, 2, { ValueKindInt, ValueKindInt }, mod_intrinsic, NULL },
  { STR_LIT("mod"),    true, 2, { ValueKindFloat, ValueKindFloat }, &mod_intrinsic, NULL },
  { STR_LIT("before"), true, 2, { ValueKindUnit, ValueKindList }, &before_intrinsic, NULL },
  { STR_LIT("after"),  true, 2, { ValueKindList, ValueKindUnit }, &after_intrinsic, NULL },
  // Comparisons
  { STR_LIT("eq"), true, 2, { ValueKindUnit, ValueKindUnit }, &eq_intrinsic, NULL },
  { STR_LIT("ne"), true, 2, { ValueKindUnit, ValueKindUnit }, &ne_intrinsic, NULL },
  { STR_LIT("ls"), true, 2, { ValueKindInt }, &ls_intrinsic, NULL },
  { STR_LIT("ls"), true, 2, { ValueKindFloat }, &ls_intrinsic, NULL },
  { STR_LIT("le"), true, 2, { ValueKindInt }, &le_intrinsic, NULL },
  { STR_LIT("le"), true, 2, { ValueKindFloat }, &le_intrinsic, NULL },
  { STR_LIT("gt"), true, 2, { ValueKindInt }, &gt_intrinsic, NULL },
  { STR_LIT("gt"), true, 2, { ValueKindFloat }, &gt_intrinsic, NULL },
  { STR_LIT("ge"), true, 2, { ValueKindInt }, &ge_intrinsic, NULL },
  { STR_LIT("ge"), true, 2, { ValueKindFloat }, &ge_intrinsic, NULL },
  // Boolean/binary
  { STR_LIT("and"), true, 2, { ValueKindInt, ValueKindInt }, &and_intrinsic, NULL },
  { STR_LIT("and"), true, 2, { ValueKindBool, ValueKindBool }, &and_intrinsic, NULL },
  { STR_LIT("or"), true, 2, { ValueKindInt, ValueKindInt }, &or_intrinsic, NULL },
  { STR_LIT("or"), true, 2, { ValueKindBool, ValueKindBool }, &or_intrinsic, NULL },
  { STR_LIT("and"), true, 2, { ValueKindInt, ValueKindInt }, &xor_intrinsic, NULL },
  { STR_LIT("and"), true, 2, { ValueKindBool, ValueKindBool }, &xor_intrinsic, NULL },
  { STR_LIT("not"), true, 1, { ValueKindUnit }, &not_intrinsic, NULL },
  // Types
  { STR_LIT("type"), true, 1, { ValueKindUnit }, &type_intrinsic, NULL },
  { STR_LIT("is-unit"), true, 1, { ValueKindUnit }, &is_unit_intrinsic, NULL },
  { STR_LIT("is-list"), true, 1, { ValueKindUnit }, &is_list_intrinsic, NULL },
  { STR_LIT("is-string"), true, 1, { ValueKindUnit }, &is_string_intrinsic, NULL },
  { STR_LIT("is-int"), true, 1, { ValueKindUnit }, &is_int_intrinsic, NULL },
  { STR_LIT("is-float"), true, 1, { ValueKindUnit }, &is_float_intrinsic, NULL },
  { STR_LIT("is-bool"), true, 1, { ValueKindUnit }, &is_bool_intrinsic, NULL },
  { STR_LIT("is-func"), true, 1, { ValueKindUnit }, &is_func_intrinsic, NULL },
  { STR_LIT("is-dict"), true, 1, { ValueKindUnit }, &is_dict_intrinsic, NULL },
  { STR_LIT("is-env"), true, 1, { ValueKindUnit }, &is_env_intrinsic, NULL },
  { STR_LIT("is-bytes"), true, 1, { ValueKindUnit }, &is_bytes_intrinsic, NULL },
  // Env
  { STR_LIT("make-env"), true, 1, { ValueKindList }, &make_env_intrinsic, NULL },
  { STR_LIT("compile"), true, 5,
    { ValueKindEnv, ValueKindString, ValueKindString,
      ValueKindList, ValueKindBool },
    &compile_intrinsic, NULL },
  { STR_LIT("eval-compiled"), false, 2,
    { ValueKindEnv, ValueKindBytes },
    &eval_compiled_intrinsic, NULL },
  { STR_LIT("eval-macros"), false, 2,
    { ValueKindEnv, ValueKindBytes },
    &eval_macros_intrinsic, NULL },
  { STR_LIT("eval"), true, 4,
    { ValueKindEnv, ValueKindString,
      ValueKindString, ValueKindList },
    &eval_intrinsic, NULL },
  // Other
  { STR_LIT("atom"), true, 1, { ValueKindList }, &atom_intrinsic, NULL },
  { STR_LIT("atom"), true, 1, { ValueKindString }, &atom_intrinsic, NULL },
  { STR_LIT("atom"), true, 1, { ValueKindDict }, &atom_intrinsic, NULL },
  { STR_LIT("atom"), true, 1, { ValueKindBytes }, &atom_intrinsic, NULL },
  { STR_LIT("copy"), true, 1, { ValueKindUnit }, &copy_intrinsic, NULL },
  { STR_LIT("exit"), false, 1, { ValueKindInt }, &exit_intrinsic, NULL },
};

u32 core_intrinsics_len = ARRAY_LEN(core_intrinsics);
