#include "aether/misc.h"
#include "lexgen/runtime.h"

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG(*(meta).file_path), (meta).row + 1, (meta).col + 1

#define PANIC(meta, text)                 \
  do {                                    \
    ERROR(META_FMT text, META_ARG(meta)); \
    vm->state = ExecStateReturn;          \
    vm->exit_code = 1;                    \
    return NULL;                          \
  } while (false)

#define PANIC_ARGS(meta, text, ...)                    \
  do {                                                 \
    ERROR(META_FMT text, META_ARG(meta), __VA_ARGS__); \
    vm->state = ExecStateReturn;                       \
    vm->exit_code = 1;                                 \
    return NULL;                                       \
  } while (false)

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

u64 value_hash(Value *value) {
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

Value **dict_get_value_root(Dict *dict, Value *key) {
  u64 index = value_hash(key) % DICT_HASH_TABLE_CAP;

  DictValue *entry = dict->items[index];
  while (entry && entry->next && !value_eq(entry->key, key))
    entry = entry->next;

  if (entry && value_eq(entry->key, key))
    return &entry->value;
  else
    return NULL;
}

Value *dict_get_value(Dict *dict, Value *key) {
  Value **root = dict_get_value_root(dict, key);

  if (root)
    return *root;
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

Value **get_child_root(Value *value, Value *key, InstrMeta *meta, Vm *vm) {
  if (value->kind == ValueKindList) {
    if (key->kind != ValueKindInt)
      PANIC(*meta, "Lists can only be indexed with integers");

    u32 j = 0;
    ListNode *node = value->as.list->next;

    while (j < key->as._int && node) {
      node = node->next;
      ++j;
    }

    if (!node)
      PANIC(*meta, "List index out of bounds\n");

    return &node->value;
  } else if (value->kind == ValueKindDict) {
    return dict_get_value_root(&value->as.dict, key);
  } else {
    StringBuilder type_sb = {0};
    sb_push_value(&type_sb, value, 0, true, true);

    Str type = sb_to_str(type_sb);

    PANIC_ARGS(*meta, "Value of type "STR_FMT" cannot be indexed\n",
               STR_ARG(type));
  }
}

void sb_push_value(StringBuilder *sb, Value *value,
                   u32 level, bool kind, bool quote_string) {
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

      sb_push_value(sb, node->value, level, kind, true);

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
    sb_push_char(sb, '\\');

    for (u32 i = 0; i < value->as.func->args.len; ++i) {
      if (i > 0)
        sb_push_char(sb, ' ');
      sb_push_str(sb, value->as.func->args.items[i]);
    }

    if (value->as.func->args.len > 0)
      sb_push_char(sb, ' ');
    sb_push(sb, "-> ...");
  } break;

  case ValueKindDict: {
    sb_push(sb, "{\n");

    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = value->as.dict.items[i];
      while (entry) {
        for (u32 j = 0; j < level + 1; ++j)
          sb_push(sb, "  ");

        sb_push_value(sb, entry->key, level + 1, kind, true);
        sb_push(sb, ": ");
        sb_push_value(sb, entry->value, level + 1, kind, true);

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
  } break;
  }
}

static void print_value(Value *value) {
  StringBuilder sb = {0};
  sb_push_value(&sb, value, 0, false, true);

  str_println(sb_to_str(sb));

  free(sb.buffer);
}

void print_instr(Instr *instr, bool hide_strings) {
  printf(META_FMT, META_ARG(instr->meta));

  switch (instr->kind) {
  case InstrKindPrimitive: {
    printf("Value ");
    if (instr->as.primitive.value->kind != ValueKindString || !hide_strings)
      print_value(instr->as.primitive.value);
    else
      printf("string\n");
  } break;

  case InstrKindFuncCall: {
    printf("Call %u\n", instr->as.func_call.args_len);
  } break;

  case InstrKindDefVar: {
    printf("Define "STR_FMT"\n", STR_ARG(instr->as.def_var.name));
  } break;

  case InstrKindGetVar: {
    printf("Get "STR_FMT"\n", STR_ARG(instr->as.get_var.name));
  } break;

  case InstrKindJump: {
    printf("Jump to "STR_FMT"\n",
           STR_ARG(instr->as.jump.label));
  } break;

  case InstrKindCondJump: {
    printf("Jump to "STR_FMT" if condition\n",
           STR_ARG(instr->as.cond_jump.label));
  } break;

  case InstrKindCondNotJump: {
    printf("Jump to "STR_FMT" if not contidion\n",
           STR_ARG(instr->as.cond_not_jump.label));
  } break;

  case InstrKindLabel: {
    printf(STR_FMT":\n", STR_ARG(instr->as.label.name));
  } break;

  case InstrKindMatchBegin: {
    printf("Match begin\n");
  } break;

  case InstrKindMatchCase: {
    printf("Match case, next or "STR_FMT"\n",
           STR_ARG(instr->as.match_case.not_label));
  } break;

  case InstrKindMatchEnd: {
    printf("Match end\n");
  } break;

  case InstrKindGet: {
    printf("Get\n");
  } break;

  case InstrKindSet: {
    printf("Set\n");
  } break;

  case InstrKindRet: {
    printf("Return\n");
  } break;

  case InstrKindList: {
    printf("Make list\n");
  } break;

  case InstrKindDict: {
    printf("Make dictionary\n");
  } break;

  case InstrKindSelf: {
    printf("Self reference\n");
  } break;
  }
}
