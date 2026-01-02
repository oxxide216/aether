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

static u64 value_hash(Value *value) {
  u64 result = 0;

  switch (value->kind) {
  case ValueKindUnit: break;

  case ValueKindList: {
    ListNode *node = value->as.list->next;
    while (node) {
      result += value_hash(node->value);
    }
  } break;

  case ValueKindString: {
    result = str_hash(value->as.string.str);
  } break;

  case ValueKindInt: {
    result = value->as._int;
  } break;

  case ValueKindFloat: {
    result = *(i64 *) &value->as._float;
  } break;

  case ValueKindBool: {
    result = value->as._bool;
  } break;

  case ValueKindDict: {
    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = value->as.dict.items[i];
      while (entry) {
        result += value_hash(entry->key);
        result += value_hash(entry->value);

        entry = entry->next;
      }
    }
  } break;

  case ValueKindFunc: break;
  case ValueKindEnv: break;

  case ValueKindBytes: {
    Str str = {
      (char *) value->as.bytes.ptr,
      value->as.bytes.len,
    };

    result = str_hash(str);
  } break;
  }

  return result;
}

Value *dict_get_value(Dict *dict, Value *key) {
  u64 index = value_hash(key) % DICT_HASH_TABLE_CAP;

  DictValue *entry = dict->items[index];
  while (entry && entry->next && !value_eq(entry->key, key))
    entry = entry->next;

  if (entry && value_eq(entry->key, key))
    return entry->value;
  else
    return NULL;
}

void dict_set_value(StackFrame *frame, Dict *dict,
                    Value *key, Value *value) {
  u64 index = value_hash(key) % DICT_HASH_TABLE_CAP;

  DictValue *entry = dict->items[index];
  while (entry && entry->next && !value_eq(entry->key, key))
    entry = entry->next;

  if (entry) {
    if (value_eq(entry->key, key)) {
      --entry->value->refs_count;
    } else {
      entry->next = arena_alloc(&frame->arena, sizeof(DictValue));
      entry = entry->next;
      entry->key = key;
    }
  } else {
    entry = arena_alloc(&frame->arena, sizeof(DictValue));
    entry->key = key;
    dict->items[index] = entry;
  }

  entry->value = value;
}

Value *dict_get_value_str_key(Dict *dict, Str string) {
  u64 index = str_hash(string) % DICT_HASH_TABLE_CAP;

  DictValue *entry = dict->items[index];
  while (entry && entry->next &&
         (entry->key->kind != ValueKindString ||
          !str_eq(entry->key->as.string.str, string)))
    entry = entry->next;

  if (entry)
    return entry->value;
  else
    return NULL;
}

void dict_set_value_str_key(StackFrame *frame, Dict *dict,
                            Str string, Value *value) {
  Value *key = value_string(string, frame);

  u64 index = str_hash(string) % DICT_HASH_TABLE_CAP;

  DictValue *entry = dict->items[index];
  while (entry && entry->next && !value_eq(entry->key, key))
    entry = entry->next;

  if (entry) {
    if (value_eq(entry->key, key)) {
      --entry->value->refs_count;
    } else {
      entry->next = arena_alloc(&frame->arena, sizeof(DictValue));
      entry = entry->next;
      entry->key = key;
    }
  } else {
    entry = arena_alloc(&frame->arena, sizeof(DictValue));
    entry->key = key;
    dict->items[index] = entry;
  }

  entry->value = value;
}

void sb_push_value(StringBuilder *sb, Value *value,
                   u32 level, bool kind,
                   bool quote_string, Vm *vm) {
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

      sb_push_value(sb, node->value, level, kind, true, vm);

      node = node->next;
    }

    sb_push_char(sb, ']');
  } break;

  case ValueKindString: {
    if (kind) {
      sb_push(sb, "str");
    } else {
      if (quote_string)
        sb_push_char(sb, '\'');

      for (u32 i = 0; i < value->as.string.str.len; ++i) {
        char _char = value->as.string.str.ptr[i];

        if (_char == '\n' && quote_string)
          sb_push(sb, "\\n");
        else
          sb_push_char(sb, _char);
      }

      if (quote_string)
        sb_push_char(sb, '\'');
    }
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

    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = value->as.dict.items[i];
      while (entry) {
        for (u32 j = 0; j < level + 1; ++j)
          sb_push(sb, "  ");

        sb_push_value(sb, entry->key, level + 1, kind, true, vm);
        sb_push(sb, ": ");
        sb_push_value(sb, entry->value, level + 1, kind, true, vm);

        sb_push_char(sb, '\n');

        entry = entry->next;
      }
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
