#include "aether/misc.h"

bool prepare_rwo_numbers(Value **a, Value **b, Vm *vm) {
  *b = value_stack_pop(&vm->stack);
  *a = value_stack_pop(&vm->stack);

  return true;
}

bool value_to_bool(Value *value) {
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
    return true;
}

void dict_push_value(Dict *dict, Value *key, Value *value) {
  DictValue dict_value = { key, value };
  DA_APPEND(*dict, dict_value);
}

void dict_push_value_str_key(RcArena *rc_arena, Dict *dict,
                             Str key, Value *value) {
  Value *string = rc_arena_alloc(rc_arena, sizeof(Value));
  *string = (Value) {
    ValueKindString,
    {
      .string = { key, (Str *) key.ptr },
    },
    0,
  };
  DictValue dict_value = { string, value };
  DA_APPEND(*dict, dict_value);
}
