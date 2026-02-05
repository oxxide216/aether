#include "aether/bytecode.h"
#include "aether/misc.h"
#include "aether/vm.h"

#define DEST_APPEND(converter, instr)               \
  DA_APPEND(converter->current_func->instrs, instr) \

typedef struct {
  Ir     dest;
  Arena *arena;
  u32    labels;
  Func  *current_func;
} Converter;

ListNode *list_clone(ListNode *nodes, StackFrame *frame) {
  if (!nodes)
    return NULL;

  ListNode *new_list = NULL;
  ListNode **new_list_next = &new_list;
  ListNode *node = nodes;
  while (node) {
    *new_list_next = arena_alloc(&frame->arena, sizeof(ListNode));
    (*new_list_next)->value = value_clone(node->value, frame);
    new_list_next = &(*new_list_next)->next;

    node = node->next;
  }

  *new_list_next = NULL;

  return new_list;
}

Dict *dict_clone(Dict *dict, StackFrame *frame) {
  Dict *copy = arena_alloc(&frame->arena, sizeof(Dict));

  for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
    DictValue *entry = dict->items[i];
    while (entry) {
      Value *key = value_clone(entry->key, frame);
      Value *value = value_clone(entry->value, frame);

      dict_set_value(frame, copy, key, value);

      entry = entry->next;
    }
  }

  return copy;
}

Value *value_unit(StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindUnit, {},
                     frame, 0, false, };
  return value;
}

Value *value_list(ListNode *nodes, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindList, { .list = nodes },
                     frame, 0, false, };
  return value;
}

Value *value_string(Str string, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindString, { .string = { string } },
                     frame, 0, false, };
  return value;
}

Value *value_bytes(Bytes bytes, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindBytes, { .bytes = bytes },
                     frame, 0, false, };
  return value;
}

Value *value_int(i64 _int, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindInt, { ._int = _int },
                     frame, 0, false, };
  return value;
}

Value *value_float(f64 _float, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindFloat, { ._float = _float },
                     frame, 0, false, };
  return value;
}

Value *value_bool(bool _bool, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindBool, { ._bool = _bool },
                     frame, 0, false, };
  return value;
}

Value *value_dict(Dict *dict, StackFrame *frame) {
  Value *value = value_alloc(frame);
  *value = (Value) { ValueKindDict, { .dict = dict },
                     frame, 0, false, };
  return value;
}

Value *value_func(FuncValue *func, StackFrame *frame) {
  Value *value = value_alloc(frame);
  func->refs_count = 1;
  *value = (Value) { ValueKindFunc, { .func = func },
                     frame, 0, false, };
  return value;
}

Value *value_env(Vm *vm, StackFrame *frame) {
  Value *value = value_alloc(frame);
  Env *env = arena_alloc(&frame->arena, sizeof(Env));
  *env = (Env) {0};
  env->vm = vm;
  env->refs_count = 1;
  *value = (Value) { ValueKindEnv, { .env = env },
                     frame, 0, false, };
  return value;
}

Value *value_alloc(StackFrame *frame) {
  Value *value = arena_alloc(&frame->arena, sizeof(Value));
  DA_APPEND(frame->values, value);

  return value;
}

Value *value_clone(Value *value, StackFrame *frame) {
  Value *copy = value_alloc(frame);
  *copy = *value;
  copy->frame = frame;
  copy->refs_count = 1;

  if (value->kind == ValueKindList) {
    copy->as.list = arena_alloc(&frame->arena, sizeof(ListNode));
    copy->as.list->next = list_clone(value->as.list->next, frame);
  } else if (value->kind == ValueKindString) {
    copy->as.string.str.len = value->as.string.str.len;
    copy->as.string.str.ptr = arena_alloc(&frame->arena, copy->as.string.str.len);
    memcpy(copy->as.string.str.ptr, value->as.string.str.ptr, copy->as.string.str.len);
  } else if (value->kind == ValueKindBytes) {
    copy->as.bytes.len = value->as.bytes.len;
    copy->as.bytes.ptr = arena_alloc(&frame->arena, copy->as.bytes.len);
    memcpy(copy->as.bytes.ptr, value->as.bytes.ptr, copy->as.bytes.len);
  } else if (value->kind == ValueKindDict) {
    copy->as.dict = dict_clone(value->as.dict, frame);
  } else if (value->kind == ValueKindFunc) {
    ++copy->as.func->refs_count;

    if (frame != value->frame) {
      copy->as.func = arena_alloc(&frame->arena, sizeof(FuncValue));
      *copy->as.func = *value->as.func;

      copy->as.func->catched_vars.items =
        arena_alloc(&frame->arena, copy->as.func->catched_vars.len *
                                   sizeof(Var));

      for (u32 i = 0; i < copy->as.func->catched_vars.len; ++i) {
        copy->as.func->catched_vars.items[i].name =
          value->as.func->catched_vars.items[i].name;
        copy->as.func->catched_vars.items[i].value =
          value_clone(value->as.func->catched_vars.items[i].value, frame);
      }
    }

    copy->as.func->cloned = true;
  } else if (value->kind == ValueKindEnv) {
    ++copy->as.env->refs_count;
    if (frame != value->frame) {
      copy->as.env = arena_alloc(&frame->arena, sizeof(Env));
      *copy->as.env = *value->as.env;
      copy->as.env->vm = arena_alloc(&frame->arena, sizeof(Vm));
      *copy->as.env->vm = *value->as.env->vm;
    }
  }

  return copy;
}

void value_free(Value *value) {
  if (value->kind == ValueKindList) {
    ListNode *node = value->as.list->next;
    while (node) {
      ListNode *next_node = node->next;
      value_free(node->value);

      node = next_node;
    }
  } else if (value->kind == ValueKindDict) {
    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry = value->as.dict->items[i];
      while (entry) {
        value_free(entry->key);
        value_free(entry->value);

        entry = entry->next;
      }
    }
  } else if (value->kind == ValueKindEnv) {
    if (--value->as.env->refs_count == 0) {
      if (value->as.env->macros.items)
        free(value->as.env->macros.items);

      if (value->as.env->included_files.items)
        free(value->as.env->included_files.items);

      if (value->as.env->include_paths.items)
        free(value->as.env->include_paths.items);

      for (u32 i = 0; i < value->as.env->cached_irs.len; ++i)
        ir_free(value->as.env->cached_irs.items[i]);

      if (value->as.env->cached_irs.items)
        free(value->as.env->cached_irs.items);

      for (u32 i = 0; i < value->as.env->cached_asts.len; ++i)
        arena_free(&value->as.env->cached_asts.items[i].arena);

      if (value->as.env->cached_asts.items)
        free(value->as.env->cached_asts.items);

      vm_destroy(value->as.env->vm);
    }
  }
}

bool value_eq(Value *a, Value *b) {
  if (a->kind != b->kind)
    return false;

  switch (a->kind) {
  case ValueKindUnit: {
    return true;
  }

  case ValueKindList: {
    ListNode *a_node = a->as.list->next;
    ListNode *b_node = b->as.list->next;

    while (a_node && b_node) {
      if (!value_eq(a_node->value, b_node->value))
        return false;

      a_node = a_node->next;
      b_node = b_node->next;
    }

    return a_node == NULL && b_node == NULL;
  }

  case ValueKindString: {
    return str_eq(a->as.string.str, b->as.string.str);
  }

  case ValueKindInt: {
    return a->as._int == b->as._int;
  } break;

  case ValueKindFloat: {
    return a->as._float == b->as._float;
  }

  case ValueKindBool: {
    return a->as._bool == b->as._bool;
  }

  case ValueKindDict: {
    for (u32 i = 0; i < DICT_HASH_TABLE_CAP; ++i) {
      DictValue *entry_a = a->as.dict->items[i];
      DictValue *entry_b = b->as.dict->items[i];

      while (entry_a && entry_b) {
        if (!value_eq(entry_a->key, entry_b->key) ||
            !value_eq(entry_a->value, entry_b->value))
          return false;
      }

      if (entry_a != entry_b)
        return false;
    }

    return true;
  }

  case ValueKindFunc: {
    if (a->as.func->intrinsic_name_id != (u16) -1)
      return a->as.func->intrinsic_name_id ==
             b->as.func->intrinsic_name_id;

    return false;
  }

  case ValueKindBytes: {
    if (a->as.bytes.len != b->as.bytes.len)
      return false;

    for (u32 i = 0; i < a->as.bytes.len; ++i)
      if (a->as.bytes.ptr[i] != b->as.bytes.ptr[i])
        return false;

    return true;
  } break;

  default: {
    return false;
  }
  }
}

void frame_free(StackFrame *frame) {
  for (u32 i = 0; i < frame->values.len; ++i)
    value_free(frame->values.items[i]);
  if (frame->values.items)
    free(frame->values.items);
  frame->values.len = 0;
  arena_free(&frame->arena);
  if (frame->vars.items)
    free(frame->vars.items);
  if (frame->match_values.items)
    free(frame->match_values.items);
  frame->vars.len = 0;
  free(frame);
}

static Expr *ast_node_clone(Expr *node, Arena *arena) {
  Expr *copy = arena_alloc(arena, sizeof(Expr));
  copy->kind = node->kind;

  switch (node->kind) {
  case ExprKindString: break;
  case ExprKindInt: break;
  case ExprKindFloat: break;
  case ExprKindBytes: break;

  case ExprKindBlock: {
    copy->as.block = ast_clone(&node->as.block, arena);
  } break;

  case ExprKindIdent: break;

  case ExprKindFunc: {
    copy->as.block = ast_clone(&node->as.block, arena);
  } break;

  case ExprKindList: {
    copy->as.list.content = ast_clone(&node->as.list.content, arena);
  } break;

  case ExprKindDict: {
    copy->as.dict.content = ast_clone(&node->as.dict.content, arena);
  } break;

  case ExprKindGet: {
    copy->as.get.chain = ast_clone(&node->as.get.chain, arena);
  } break;

  case ExprKindSet: {
    copy->as.set.parent = ast_node_clone(node->as.set.parent, arena);
    copy->as.set.key = ast_node_clone(node->as.set.key, arena);
    copy->as.set.new = ast_node_clone(node->as.set.new, arena);
  } break;

  case ExprKindSetVar: {
    copy->as.set_var.new = ast_node_clone(node->as.set_var.new, arena);
  } break;

  case ExprKindFuncCall: {
    copy->as.func_call.func = ast_node_clone(node->as.func_call.func, arena);
    copy->as.func_call.args = ast_clone(&node->as.func_call.args, arena);
  } break;

  case ExprKindLet: {
    copy->as.let.value = ast_node_clone(node->as.let.value, arena);
  } break;

  case ExprKindRet: {
    copy->as.ret.value = ast_node_clone(node->as.ret.value, arena);
  } break;

  case ExprKindIf: {
    copy->as._if.cond = ast_node_clone(node->as._if.cond, arena);
    copy->as._if.if_body = ast_clone(&node->as._if.if_body, arena);
    copy->as._if.else_body = ast_clone(&node->as._if.else_body, arena);
  } break;

  case ExprKindMatch: {
    copy->as.match.value = ast_node_clone(node->as.match.value, arena);

    Branches branches_copy;
    branches_copy.len = node->as.match.branches.len;
    branches_copy.cap = branches_copy.len;
    branches_copy.items = arena_alloc(arena, branches_copy.cap * sizeof(Branch));

    for (u32 i = 0; i < branches_copy.len; ++i) {
      Branch *branch = node->as.match.branches.items + i;

      branches_copy.items[i].value = ast_node_clone(branch->value, arena);
      branches_copy.items[i].body = ast_node_clone(branch->body, arena);
    }

    copy->as.match.branches = branches_copy;

    return copy;
  } break;

  case ExprKindSelf: break;
  }

  return copy;
}

Exprs ast_clone(Exprs *ast, Arena *arena) {
  Exprs copy;
  copy.len = ast->len;
  copy.items = arena_alloc(arena, copy.len * sizeof(Expr *));

  for (u32 i = 0; i < ast->len; ++i)
    copy.items[i] = ast_node_clone(ast->items[i], arena);

  return copy;
}

static void ast_block_to_ir(Converter *converter, Exprs *block);

static void ast_node_to_ir(Converter *converter, Expr *node) {
  switch (node->kind) {
  case ExprKindString: {
    Instr instr = {0};
    instr.kind = InstrKindString;
    instr.as.string.string_id = node->as.string.string_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindInt: {
    Instr instr = {0};
    instr.kind = InstrKindInt;
    instr.as._int._int = node->as._int._int;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindFloat: {
    Instr instr = {0};
    instr.kind = InstrKindFloat;
    instr.as._float._float = node->as._float._float;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindBytes: {
    Instr instr = {0};
    instr.kind = InstrKindBytes;
    instr.as.bytes.bytes = node->as.bytes.bytes;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindBlock: {
    ast_block_to_ir(converter, &node->as.block);
  } break;

  case ExprKindIdent: {
    Instr instr = {0};
    instr.kind = InstrKindGetVar;
    instr.as.get_var.name_id = node->as.ident.name_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindFunc: {
    Instr instr = {0};
    instr.kind = InstrKindFunc;
    instr.as.func.args = node->as.func.args;
    instr.as.func.intrinsic_name_id = node->as.func.intrinsic_name_id;
    instr.meta = node->meta;

    if (node->as.func.intrinsic_name_id == (u16) -1) {
      Func *prev_func = converter->current_func;

      converter->current_func = arena_alloc(converter->arena, sizeof(Func));
      converter->current_func->args = node->as.func.args;

      ast_block_to_ir(converter, &node->as.func.body);

      instr.as.func.body = converter->current_func;

      converter->current_func = prev_func;
    }

    DEST_APPEND(converter, instr);
  } break;

  case ExprKindList: {
    ast_block_to_ir(converter, &node->as.list.content);

    Instr instr = {0};
    instr.kind = InstrKindList;
    instr.as.list.len = node->as.list.content.len;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindDict: {
    ast_block_to_ir(converter, &node->as.dict.content);

    Instr instr = {0};
    instr.kind = InstrKindDict;
    instr.as.dict.len = node->as.dict.content.len / 2;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindGet: {
    ast_block_to_ir(converter, &node->as.get.chain);

    Instr instr = {0};
    instr.kind = InstrKindGet;
    instr.as.get.chain_len = node->as.get.chain.len;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindSet: {
    ast_node_to_ir(converter, node->as.set.parent);
    ast_node_to_ir(converter, node->as.set.key);
    ast_node_to_ir(converter, node->as.set.new);

    Instr instr = {0};
    instr.kind = InstrKindSet;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindSetVar: {
    ast_node_to_ir(converter, node->as.set_var.new);

    Instr instr = {0};
    instr.kind = InstrKindSetVar;
    instr.as.set_var.name_id = node->as.set_var.name_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindFuncCall: {
    ast_node_to_ir(converter, node->as.func_call.func);

    u32 args_begin = converter->current_func->instrs.len;

    ast_block_to_ir(converter, &node->as.func_call.args);

    u32 args_end = converter->current_func->instrs.len;

    Instr instr = {0};
    instr.kind = InstrKindFuncCall;
    instr.as.func_call.args_len = node->as.func_call.args.len;
    instr.as.func_call.args_instrs_len = args_end - args_begin;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindLet: {
    ast_node_to_ir(converter, node->as.let.value);

    Instr instr = {0};
    instr.kind = InstrKindDefVar;
    instr.as.def_var.name_id = node->as.let.name_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindRet: {
    if (node->as.ret.value)
      ast_node_to_ir(converter, node->as.ret.value);

    Instr instr = {0};
    instr.kind = InstrKindRet;
    instr.as.ret.has_value = node->as.ret.value != NULL;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindIf: {
    StringBuilder sb = {0};
    sb_push_char(&sb, 'l');

    sb_push_u32(&sb, converter->labels++);
    u16 else_label_id = copy_str(sb_to_str(sb));

    sb.len = 1;

    sb_push_u32(&sb, converter->labels++);
    u16 end_label_id = copy_str(sb_to_str(sb));

    free(sb.buffer);

    ast_node_to_ir(converter, node->as._if.cond);

    Instr instr = {0};
    instr.kind = InstrKindCondNotJump;
    instr.as.cond_not_jump.label_id = else_label_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);

    ast_block_to_ir(converter, &node->as._if.if_body);

    instr.kind = InstrKindJump;
    instr.as.jump.label_id = end_label_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);

    instr.kind = InstrKindLabel;
    instr.as.label.name_id = else_label_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);

    ast_block_to_ir(converter, &node->as._if.else_body);

    instr.kind = InstrKindLabel;
    instr.as.label.name_id = end_label_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindMatch: {
    StringBuilder sb = {0};
    sb_push_char(&sb, 'l');

    sb_push_u32(&sb, converter->labels++);
    u16 end_label_id = copy_str(sb_to_str(sb));

    ast_node_to_ir(converter, node->as.match.value);

    Instr instr = {0};
    instr.kind = InstrKindMatchBegin;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);

    for (u32 i = 0; i < node->as.match.branches.len; ++i) {
      sb.len = 1;

      sb_push_u32(&sb, converter->labels++);
      u16 next_label_id = copy_str(sb_to_str(sb));

      if (node->as.match.branches.items[i].value) {
        ast_node_to_ir(converter, node->as.match.branches.items[i].value);

        instr.kind = InstrKindMatchCase;
        instr.as.match_case.not_label_id = next_label_id;
        instr.meta = node->meta;
        DEST_APPEND(converter, instr);
      }

      ast_node_to_ir(converter, node->as.match.branches.items[i].body);

      instr.kind = InstrKindJump;
      instr.as.jump.label_id = end_label_id;
      instr.meta = node->meta;
      DEST_APPEND(converter, instr);

      if (node->as.match.branches.items[i].value) {
        instr.kind = InstrKindLabel;
        instr.as.label.name_id = next_label_id;
        instr.meta = node->meta;
        DEST_APPEND(converter, instr);
      }
    }

    free(sb.buffer);

    instr.kind = InstrKindLabel;
    instr.as.label.name_id = end_label_id;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);

    instr.kind = InstrKindMatchEnd;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;

  case ExprKindSelf: {
    Instr instr = {0};
    instr.kind = InstrKindSelf;
    instr.meta = node->meta;
    DEST_APPEND(converter, instr);
  } break;
  }
}

static void ast_block_to_ir(Converter *converter, Exprs *block) {
  for (u32 i = 0; i + 1 < block->len; ++i) {
    ast_node_to_ir(converter, block->items[i]);
  }

  if (block->len > 0)
    ast_node_to_ir(converter, block->items[block->len - 1]);
}

Ir ast_to_ir(Exprs *ast, Arena *arena) {
  Ir dest = arena_alloc(arena, sizeof(Func));
  Converter converter = { dest, arena, 0, dest };

  for (u32 i = 0; i + 1 < ast->len; ++i)
    ast_node_to_ir(&converter, ast->items[i]);

  if (ast->len > 0)
    ast_node_to_ir(&converter, ast->items[ast->len - 1]);

  return converter.dest;
}

void ir_free(Ir ir) {
  for (u32 i = 0; i < ir->instrs.len; ++i)
    if (ir->instrs.items[i].kind == InstrKindFunc &&
        ir->instrs.items[i].as.func.intrinsic_name_id == (u16) -1)
      ir_free(ir->instrs.items[i].as.func.body);

  for (u32 i = 0; i < LABELS_HASH_TABLE_CAP; ++i) {
    Label *entry = ir->labels.items[i];

    while (entry) {
      Label *prev = entry;
      entry = entry->next;
      free(prev);
    }
  }

  free(ir->instrs.items);
}
