#include "aether/misc.h"

bool value_to_bool(Value *value) {
  if (value->kind == ValueKindUnit)
    return false;
  else if (value->kind == ValueKindList)
    return value->as.list->next != NULL;
  else if (value->kind == ValueKindString)
    return value->as.string.len != 0;
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

void dict_push_value_str_key(Arena *arena, Dict *dict,
                             Str key, Value *value) {
  Value *string = arena_alloc(arena, sizeof(Value));
  *string = (Value) {
    ValueKindString,
    { .string = key },
  };
  DictValue dict_value = { string, value };
  DA_APPEND(*dict, dict_value);
}

Value *dict_get_value_str_key(Arena *arena, Dict *dict, Str key) {
  for (u32 i = 0; i < dict->len; ++i)
    if (dict->items[i].key->kind == ValueKindString &&
        str_eq(dict->items[i].key->as.string, key))
      return dict->items[i].value;

  Value *result = arena_alloc(arena, sizeof(Value));
  result->kind = ValueKindUnit;
  return result;
}

void sb_push_value(StringBuilder *sb, Value *value, u32 level, bool kind) {
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
      sb_push_value(sb, node->value, level, kind);
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
      sb_push_str(sb, value->as.string);
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
      sb_push(sb, "str");
    } else {
      if (value->as._bool)
        sb_push(sb, "true");
      else
        sb_push(sb, "false");
    }
  } break;

  case ValueKindFunc: {
    sb_push_char(sb, '[');

    for (u32 i = 0; i < value->as.func.args.len; ++i) {
      if (i > 0)
        sb_push_char(sb, ' ');
      sb_push_str(sb, value->as.func.args.items[i]);
    }

    sb_push(sb, "] -> ...");
  } break;

  case ValueKindDict: {
    sb_push(sb, "{\n");

    for (u32 i = 0; i < value->as.dict.len; ++i) {
      for (u32 j = 0; j < level + 1; ++j)
        sb_push(sb, "  ");

      sb_push_value(sb, value->as.dict.items[i].key, level + 1, kind);
      sb_push(sb, ": ");
      sb_push_value(sb, value->as.dict.items[i].value, level + 1, kind);

      sb_push_char(sb, '\n');
    }

    for (u32 j = 0; j < level; ++j)
      sb_push(sb, "  ");
    sb_push_char(sb, '}');
  } break;

  case ValueKindEnv: {
    sb_push(sb, "environment");
  } break;
  }
}
