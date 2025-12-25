#include "aether/misc.h"

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
  else if (value->kind == ValueKindBytes)
    return value->as.bytes.len != 0 &&
           ((u8 *) value->as.bytes.ptr)[0];
  else
    return true;
}

void dict_push_value(Dict *dict, Value *key, Value *value) {
  DictValue dict_value = { key, value };
  DA_APPEND(*dict, dict_value);
}

void dict_push_value_str_key(StackFrame *frame, Dict *dict, Str string, Value *value) {
  if (dict->len == dict->cap) {
    if (dict->cap == 0)
      dict->cap = 1;
    else
      dict->cap *= 2;
    DictValue *new_items = arena_alloc(&frame->arena, dict->cap * sizeof(DictValue));
    memcpy(new_items, dict->items, dict->len * sizeof(DictValue));
    dict->items = new_items;
  }

  Value *key = value_alloc(frame);
  *key = (Value) {
    ValueKindString,
    { .string = { string } },
    frame, 1, false,
  };
  DictValue dict_value = { key, value };
  dict->items[dict->len++] = dict_value;
}

Value *dict_get_value_str_key(StackFrame *frame, Dict *dict, Str string) {
  for (u32 i = 0; i < dict->len; ++i)
    if (dict->items[i].key->kind == ValueKindString &&
        str_eq(dict->items[i].key->as.string.str, string))
      return dict->items[i].value;

  Value *result = value_alloc(frame);
  result->kind = ValueKindUnit;
  return result;
}

void sb_push_value(StringBuilder *sb, Value *value,
                   u32 level, bool kind, Vm *vm) {
  switch (value->kind) {
  case ValueKindUnit: {
    sb_push(sb, "unit");
  } break;

  case ValueKindList: {
    sb_push_char(sb, '[');

    ListNode *node = value->as.list->next;
    while (node) {
      if (node != value->as.list->next)
        sb_push_char(sb, ' ');

      if (node->value->kind == ValueKindString)
        sb_push_char(sb, '\'');
      sb_push_value(sb, node->value, level, kind, vm);
      if (node->value->kind == ValueKindString)
        sb_push_char(sb, '\'');

      node = node->next;
    }

    sb_push_char(sb, ']');
  } break;

  case ValueKindString: {
    if (kind)
      sb_push(sb, "str");
    else
      sb_push_str(sb, value->as.string.str);
  } break;

  case ValueKindInt: {
    if (kind)
      sb_push(sb, "int");
    else
      sb_push_i64(sb, value->as._int);
  } break;

  case ValueKindFloat: {
    if (kind)
      sb_push(sb, "float");
    else
      sb_push_f64(sb, value->as._float);
  } break;

  case ValueKindBool: {
    if (kind) {
      sb_push(sb, "bool");
    } else {
      if (value->as._bool)
        sb_push(sb, "true");
      else
        sb_push(sb, "false");
    }
  } break;

  case ValueKindFunc: {
    sb_push_char(sb, '[');

    for (u32 i = 0; i < value->as.func->args.len; ++i) {
      if (i > 0)
        sb_push_char(sb, ' ');
      sb_push_str(sb, value->as.func->args.items[i]);
    }

    sb_push(sb, "] -> ...");
  } break;

  case ValueKindDict: {
    sb_push(sb, "{\n");

    for (u32 i = 0; i < value->as.dict.len; ++i) {
      for (u32 j = 0; j < level + 1; ++j)
        sb_push(sb, "  ");

      sb_push_value(sb, value->as.dict.items[i].key, level + 1, kind, vm);
      sb_push(sb, ": ");
      sb_push_value(sb, value->as.dict.items[i].value, level + 1, kind, vm);

      sb_push_char(sb, '\n');
    }

    for (u32 j = 0; j < level; ++j)
      sb_push(sb, "  ");
    sb_push_char(sb, '}');
  } break;

  case ValueKindEnv: {
    sb_push(sb, "environment");
  } break;

  case ValueKindBytes: {
    if (kind) {
      sb_push(sb, "bytes");
    } else {
      Str bytes_string = {
        (char *) value->as.bytes.ptr,
        value->as.bytes.len,
      };

      sb_push_str(sb, bytes_string);
    }
  } break;

  default: {
    ERROR("Unknown value kind: %u\n", value->kind);
    vm->state = ExecStateExit;
    vm->exit_code = 1;
  } break;
  }
}
