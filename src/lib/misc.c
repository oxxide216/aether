#include "aether/misc.h"
#include "lexgen/runtime.h"

#define META_FMT       STR_FMT":%u:%u: "
#define META_ARG(meta) STR_ARG(*(meta).file_path), (meta).row + 1, (meta).col + 1

#define PANIC(meta, text)                 \
  do {                                    \
    ERROR(META_FMT text, META_ARG(meta)); \
    vm->state = ExecStateExit;            \
    vm->exit_code = 1;                    \
    return NULL;                          \
  } while (false)

#define PANIC_ARGS(meta, text, ...)                    \
  do {                                                 \
    ERROR(META_FMT text, META_ARG(meta), __VA_ARGS__); \
    vm->state = ExecStateExit;                         \
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
    result = (u64) value->as._int;
  } break;

  case ValueKindFloat: {
    result = (u64) value->as._float;
  } break;

  case ValueKindBool: {
    result = value->as._bool;
  } break;

  case ValueKindDict: {
    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = value->as.dict->items[i];
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
      PANIC(*meta, "Lists can only be indexed with integers\n");

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
    return dict_get_value_root(value->as.dict, key);
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
      sb_push(sb, "string");
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

      Str arg_name = get_str(value->as.func->args.items[i]);
      sb_push_str(sb, arg_name);
    }

    if (value->as.func->args.len > 0)
      sb_push_char(sb, ' ');

    if (value->as.func->intrinsic_name_id == (u16) -1) {
      sb_push(sb, "-> ...");
    } else {
      Str intrinsic_name = get_str(value->as.func->intrinsic_name_id);
      sb_push_str(sb, intrinsic_name);
    }
  } break;

  case ValueKindDict: {
    sb_push(sb, "{\n");

    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = value->as.dict->items[i];
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

void print_instr(Instr *instr, bool hide_strings) {
  printf(META_FMT, META_ARG(instr->meta));

  switch (instr->kind) {
  case InstrKindString: {
    printf("Value ");

    if (!hide_strings) {
      Str string = get_str(instr->as.string.string_id);
      str_println(string);
    } else {
      printf("string\n");
    }
  } break;

  case InstrKindInt: {
    printf("Value ");
    printf("%ld\n", instr->as._int._int);
  } break;

  case InstrKindFloat: {
    printf("Value ");
    printf("%lf\n", instr->as._float._float);
  } break;

  case InstrKindBytes: {
    printf("Value ");
    printf("%.*s",
           instr->as.bytes.bytes.len,
           (char *) instr->as.bytes.bytes.ptr);
  } break;

  case InstrKindFunc: {
    printf("\\");

    for (u32 i = 0; i < instr->as.func.args.len; ++i) {
      if (i > 0)
        printf(" ");

      Str arg_name = get_str(instr->as.func.args.items[i]);
      str_print(arg_name);
    }

    if (instr->as.func.args.len > 0)
      printf(" ");

    if (instr->as.func.intrinsic_name_id == (u16) -1) {
      printf("-> ...\n");
    } else {
      Str intrinsic_name = get_str(instr->as.func.intrinsic_name_id);
      printf("-> "STR_FMT"\n", STR_ARG(intrinsic_name));
    }
  } break;

  case InstrKindFuncCall: {
    printf("Call %u\n", instr->as.func_call.args_len);
  } break;

  case InstrKindDefVar: {
    Str name = get_str(instr->as.def_var.name_id);
    printf("Define "STR_FMT"\n", STR_ARG(name));
  } break;

  case InstrKindGetVar: {
    Str name = get_str(instr->as.get_var.name_id);
    printf("Get "STR_FMT"\n", STR_ARG(name));
  } break;

  case InstrKindSetVar: {
    Str name = get_str(instr->as.get_var.name_id);
    printf("Set "STR_FMT"\n", STR_ARG(name));
  } break;

  case InstrKindJump: {
    Str label = get_str(instr->as.jump.label_id);
    printf("Jump to "STR_FMT"\n", STR_ARG(label));
  } break;

  case InstrKindCondJump: {
    Str label = get_str(instr->as.cond_jump.label_id);
    printf("Jump to "STR_FMT" if condition\n", STR_ARG(label));
  } break;

  case InstrKindCondNotJump: {
    Str label = get_str(instr->as.cond_not_jump.label_id);
    printf("Jump to "STR_FMT" if not contidion\n", STR_ARG(label));
  } break;

  case InstrKindLabel: {
    Str name = get_str(instr->as.label.name_id);
    printf(STR_FMT":\n", STR_ARG(name));
  } break;

  case InstrKindMatchBegin: {
    printf("Match begin\n");
  } break;

  case InstrKindMatchCase: {
    Str not_label = get_str(instr->as.match_case.not_label_id);
    printf("Match case, next or "STR_FMT"\n", STR_ARG(not_label));
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
    printf("Make list of length %u\n", instr->as.list.len);
  } break;

  case InstrKindDict: {
    printf("Make dictionary of length %u\n", instr->as.dict.len);
  } break;

  case InstrKindSelf: {
    printf("Self reference\n");
  } break;
  }
}

void print_value(Value *value, bool kind) {
  StringBuilder sb = {0};
  sb_push_value(&sb, value, 0, kind, true);

  str_println(sb_to_str(sb));

  free(sb.buffer);
}

Value *get_from_string(Vm *vm, Str string, i64 index) {
  u32 begin_byte = 0;
  u32 current_index = 0;
  u32 bytes_len = 0;
  u32 wchar_len;
  bool found = false;

  while (get_next_wchar(string, current_index, &wchar_len) != '\0') {
    if (current_index == index) {
      begin_byte = bytes_len;
      found = true;
    }

    ++current_index;
    bytes_len += wchar_len;

    if (found)
      break;
  }

  if (!found)
    return NULL;

  Str sub_string = {
    string.ptr + begin_byte,
    bytes_len - begin_byte,
  };

  return value_string(sub_string, vm->current_frame);
}
