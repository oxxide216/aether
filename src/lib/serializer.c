#include "aether/serializer.h"
#include "shl/shl-log.h"

typedef struct {
  u32 a, b;
} IndexPair;

typedef Da(IndexPair) IndicesMap;

static void reserve_space(u32 amount, u8 **data, u32 *data_size, u32 *end) {
  u32 new_data_size = *data_size;
  while (*end + amount > new_data_size)
    new_data_size *= 2;

  if (new_data_size != *data_size) {
    *data_size = new_data_size;
    *data = realloc(*data, *data_size);
  }
}

static void serialize_str_raw(Str *str, u8 **data, u32 *data_size, u32 *end) {
  reserve_space(sizeof(u32) + str->len, data, data_size, end);
  *(u32 *) (*data + *end) = str->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < str->len; ++i) {
    *(char *) (*data + *end) = str->ptr[i];
    *end += sizeof(char);
  }
}

static void serialize_str(u16 id, u8 **data, u32 *data_size, u32 *end) {
  Str *str = get_str(id);
  serialize_str_raw(str, data, data_size, end);
}

static void serialize_instrs(Instrs *instrs, u8 **data, u32 *data_size,
                             u32 *end, FilePathOffsets *path_offsets,
                             IndicesMap *map) {
  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = instrs->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < instrs->len; ++i) {
    Instr *instr = instrs->items + i;

    reserve_space(sizeof(InstrKind), data, data_size, end);
    *(InstrKind *) (*data + *end) = instr->kind;
    *end += sizeof(InstrKind);

    switch (instr->kind) {
    case InstrKindString: {
      serialize_str(instr->as.string.string_id, data, data_size, end);
    } break;

    case InstrKindInt: {
      reserve_space(sizeof(i64), data, data_size, end);
      *(i64 *) (*data + *end) = instr->as._int._int;
      *end += sizeof(i64);
    } break;

    case InstrKindFloat: {
      reserve_space(sizeof(f64), data, data_size, end);
      *(f64 *) (*data + *end) = instr->as._float._float;
      *end += sizeof(f64);
    } break;

    case InstrKindBytes: {
      Str bytes_str = {
        (char *) instr->as.bytes.bytes.ptr,
        instr->as.bytes.bytes.len,
      };

      serialize_str_raw(&bytes_str, data, data_size, end);
    } break;

    case InstrKindFunc: {
      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = instr->as.func.args.len;
      *end += sizeof(u32);

      for (u32 j = 0; j < instr->as.func.args.len; ++j)
        serialize_str(instr->as.func.args.items[j], data, data_size, end);

      u32 new_index = instr->as.func.body_index;

      for (u32 j = 0; j < map->len; ++j) {
        if (map->items[j].a == instr->as.func.body_index) {
          new_index = map->items[j].b;

          break;
        }
      }

      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = new_index;
      *end += sizeof(u32);

      reserve_space(sizeof(u8), data, data_size, end);
      *(u8 *) (*data + *end) = instr->as.func.intrinsic_name_id != (u16) -1;
      *end += sizeof(u8);

      if (instr->as.func.intrinsic_name_id != (u16) -1)
        serialize_str(instr->as.func.intrinsic_name_id, data, data_size, end);
    } break;

    case InstrKindFuncCall: {
      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = instr->as.func_call.args_len;
      *end += sizeof(u32);

      reserve_space(sizeof(u8), data, data_size, end);
      *(u8 *) (*data + *end) = instr->as.func_call.value_ignored;
      *end += sizeof(u8);
    } break;

    case InstrKindDefVar: {
      serialize_str(instr->as.def_var.name_id, data, data_size, end);
    } break;

    case InstrKindGetVar: {
      serialize_str(instr->as.get_var.name_id, data, data_size, end);
    } break;

    case InstrKindSetVar: {
      serialize_str(instr->as.set_var.name_id, data, data_size, end);
    } break;

    case InstrKindJump: {
      serialize_str(instr->as.jump.label_id, data, data_size, end);
    } break;

    case InstrKindCondJump: {
      serialize_str(instr->as.cond_jump.label_id, data, data_size, end);
    } break;

    case InstrKindCondNotJump: {
      serialize_str(instr->as.cond_not_jump.label_id, data, data_size, end);
    } break;

    case InstrKindLabel: {
      serialize_str(instr->as.label.name_id, data, data_size, end);
    } break;

    case InstrKindMatchBegin: break;

    case InstrKindMatchCase: {
      serialize_str(instr->as.match_case.not_label_id, data, data_size, end);
    } break;

    case InstrKindMatchEnd: break;

    case InstrKindGet: {
      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = instr->as.get.chain_len;
      *end += sizeof(u32);
    } break;

    case InstrKindSet: break;

    case InstrKindRet: {
      reserve_space(sizeof(u8), data, data_size, end);
      *(u8 *) (*data + *end) = instr->as.ret.has_value;
      *end += sizeof(u8);
    } break;

    case InstrKindList: {
      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = instr->as.list.len;
      *end += sizeof(u32);
    } break;

    case InstrKindDict: {
      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = instr->as.dict.len;
      *end += sizeof(u32);
    } break;

    case InstrKindSelf: break;
    }


    bool found = false;

    for (u32 j = 0; j < path_offsets->len; ++j) {
      if (str_eq(path_offsets->items[j].path, *instr->meta.file_path)) {
        reserve_space(sizeof(u32), data, data_size, end);
        *(u32 *) (*data + *end) = j;
        *end += sizeof(u32);

        found = true;
        break;
      }
    }

    if (!found) {
      ERROR("Could not find offset for "STR_FMT"\n",
            STR_ARG(*instr->meta.file_path));
      exit(1);
    }

    reserve_space(sizeof(u16) * 2, data, data_size, end);
    *(u16 *) (*data + *end) = instr->meta.row;
    *end += sizeof(u16);
    *(u16 *) (*data + *end) = instr->meta.col;
    *end += sizeof(u16);
  }
}

static void serialize_included_files(u8 **data, u32 *data_size, u32 *end,
                                     FilePaths *included_files,
                                     FilePathOffsets *path_offsets) {
  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = included_files->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < included_files->len; ++i) {
    FilePathOffset path_offset = { *included_files->items[i], *end };
    DA_APPEND(*path_offsets, path_offset);

    serialize_str_raw(included_files->items[i], data, data_size, end);
  }
}

u8 *serialize(Ir *ir, u32 *size, FilePaths *included_files) {
  *size = sizeof(u32) * 2;
  u32 data_size = sizeof(u32) * 2;
  u8 *data = malloc(data_size);

  FilePathOffsets path_offsets = {0};
  serialize_included_files(&data, &data_size, size, included_files, &path_offsets);

  u32 ir_len_end = *size;
  u32 funcs_skipped = 0;
  IndicesMap map = {0};
  u32 len = 0;

  for (u32 i = 0; i < ir->len; ++i) {
    IndexPair pair = {
      i,
      i - funcs_skipped,
    };
    DA_APPEND(map, pair);
  }

  reserve_space(sizeof(u32), &data, &data_size, size);
  *size += sizeof(u32);

  for (u32 i = 0; i < ir->len; ++i) {
    Func *func = ir->items + i;

    reserve_space(sizeof(u32), &data, &data_size, size);
    *(u32 *) (data + *size) = func->args.len;
    *size += sizeof(u32);

    for (u32 j = 0; j < func->args.len; ++j)
      serialize_str(func->args.items[j], &data, &data_size, size);

    serialize_instrs(&func->instrs, &data, &data_size,
                     size, &path_offsets, &map);

    ++len;
  }

  *(u32 *) (data + ir_len_end) = len;

  if (path_offsets.items)
    free(path_offsets.items);

  *(u32 *) data = *(u32 *) "ABC\0";
  *(u32 *) (data + sizeof(u32)) = *size;

  return data;
}

static void serialize_ast(Exprs *ast, u8 **data, u32 *data_size,
                          u32 *end, FilePathOffsets *path_offsets,
                          Str *file_path, IndicesMap *map);

static void serialize_ast_node(Expr *node, u8 **data, u32 *data_size,
                               u32 *end, FilePathOffsets *path_offsets,
                               Str *file_path, IndicesMap *map) {
  reserve_space(sizeof(ExprKind), data, data_size, end);
  *(ExprKind *) (*data + *end) = node->kind;
  *end += sizeof(ExprKind);

  switch (node->kind) {
  case ExprKindString: {
    serialize_str(node->as.string.string_id, data, data_size, end);
  } break;

  case ExprKindInt: {
    reserve_space(sizeof(i64), data, data_size, end);
    *(i64 *) (*data + *end) = node->as._int._int;
    *end += sizeof(i64);
  } break;

  case ExprKindFloat: {
    reserve_space(sizeof(f64), data, data_size, end);
    *(f64 *) (*data + *end) = node->as._float._float;
    *end += sizeof(f64);
  } break;

  case ExprKindBytes: {
    Str bytes_str = {
      (char *) node->as.bytes.bytes.ptr,
      node->as.bytes.bytes.len,
    };

    serialize_str_raw(&bytes_str, data, data_size, end);
  } break;

  case ExprKindBlock: {
    serialize_ast(&node->as.block, data, data_size, end,
                  path_offsets, file_path, map);
  } break;

  case ExprKindIdent: {
    serialize_str(node->as.ident.name_id, data, data_size, end);
  } break;

  case ExprKindFunc: {
    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = node->as.func.args.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < node->as.func.args.len; ++i)
      serialize_str(node->as.func.args.items[i], data, data_size, end);

    serialize_ast(&node->as.func.body, data, data_size,
                  end, path_offsets, file_path, map);

    reserve_space(sizeof(u8), data, data_size, end);
    *(u8 *) (*data + *end) = node->as.func.intrinsic_name_id != (u16) -1;
    *end += sizeof(u8);

    if (node->as.func.intrinsic_name_id != (u16) -1)
      serialize_str(node->as.func.intrinsic_name_id, data, data_size, end);
  } break;

  case ExprKindList: {
    serialize_ast(&node->as.list.content, data, data_size,
                  end, path_offsets, file_path, map);
  } break;

  case ExprKindDict: {
    serialize_ast(&node->as.dict.content, data, data_size,
                  end, path_offsets, file_path, map);
  } break;

  case ExprKindGet: {
    serialize_ast(&node->as.get.chain, data, data_size,
                  end, path_offsets, file_path, map);
  } break;

  case ExprKindSet: {
    serialize_ast_node(node->as.set.parent, data, data_size,
                       end, path_offsets, file_path, map);
    serialize_ast_node(node->as.set.key, data, data_size,
                       end, path_offsets, file_path, map);
    serialize_ast_node(node->as.set.new, data, data_size,
                       end, path_offsets, file_path, map);
  } break;

  case ExprKindSetVar: {
    serialize_str(node->as.set_var.name_id, data, data_size, end);

    serialize_ast_node(node->as.set_var.new, data, data_size,
                       end, path_offsets, file_path, map);
  } break;

  case ExprKindFuncCall: {
    serialize_ast_node(node->as.func_call.func, data, data_size,
                       end, path_offsets, file_path, map);
    serialize_ast(&node->as.func_call.args, data, data_size,
                  end, path_offsets, file_path, map);

    reserve_space(sizeof(u8), data, data_size, end);
    *(u8 *) (*data + *end) = node->as.func_call.value_ignored;
    *end += sizeof(u8);
  } break;

  case ExprKindLet: {
    serialize_str(node->as.let.name_id, data, data_size, end);
    serialize_ast_node(node->as.let.value, data, data_size,
                       end, path_offsets, file_path, map);
  } break;

  case ExprKindRet: {
    reserve_space(sizeof(u8), data, data_size, end);
    *(u8 *) (*data + *end) = node->as.ret.value != NULL;
    *end += sizeof(u8);

    if (node->as.ret.value)
      serialize_ast_node(node->as.ret.value, data, data_size,
                         end, path_offsets, file_path, map);
  } break;

  case ExprKindIf: {
    serialize_ast_node(node->as._if.cond, data, data_size,
                       end, path_offsets, file_path, map);
    serialize_ast(&node->as._if.if_body, data, data_size,
                  end, path_offsets, file_path, map);
    serialize_ast(&node->as._if.else_body, data, data_size,
                  end, path_offsets, file_path, map);
  } break;

  case ExprKindWhile: {
    serialize_ast_node(node->as._while.cond, data, data_size,
                       end, path_offsets, file_path, map);
    serialize_ast(&node->as._while.body, data, data_size,
                  end, path_offsets, file_path, map);
  } break;

  case ExprKindMatch: {
    serialize_ast_node(node->as.match.value, data, data_size,
                       end, path_offsets, file_path, map);

    for (u32 j = 0; j < node->as.match.branches.len; ++j) {
      Branch *branch = node->as.match.branches.items + j;

      serialize_ast_node(branch->value, data, data_size,
                         end, path_offsets, file_path, map);
      serialize_ast_node(branch->body, data, data_size,
                         end, path_offsets, file_path, map);
    }
  } break;

  case ExprKindSelf: break;
  }

  bool found = false;

  for (u32 j = 0; j < path_offsets->len; ++j) {
    if (str_eq(path_offsets->items[j].path, *node->meta.file_path)) {
      reserve_space(sizeof(u32), data, data_size, end);
      *(u32 *) (*data + *end) = j;
      *end += sizeof(u32);

      found = true;
      break;
    }
  }

  if (!found) {
    ERROR("Could not find offset for "STR_FMT"\n",
          STR_ARG(*node->meta.file_path));
    exit(1);
  }

  reserve_space(sizeof(u16) * 2, data, data_size, end);
  *(u16 *) (*data + *end) = node->meta.row;
  *end += sizeof(u16);
  *(u16 *) (*data + *end) = node->meta.col;
  *end += sizeof(u16);
}

static void serialize_ast(Exprs *ast, u8 **data, u32 *data_size,
                          u32 *end, FilePathOffsets *path_offsets,
                          Str *file_path, IndicesMap *map) {
  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = ast->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < ast->len; ++i)
    serialize_ast_node(ast->items[i], data, data_size,
                       end, path_offsets, file_path, map);
}

u8 *serialize_macros(Macros *macros, u32 *size, FilePaths *included_files) {
  *size = sizeof(u32) * 2;
  u32 data_size = sizeof(u32) * 2;
  u8 *data = malloc(data_size);

  FilePathOffsets path_offsets = {0};
  serialize_included_files(&data, &data_size, size, included_files, &path_offsets);

  reserve_space(sizeof(u32), &data, &data_size, size);
  *(u32 *) (data + *size) = macros->len;
  *size += sizeof(u32);

  for (u32 i = 0; i < macros->len; ++i) {
    Macro *macro = macros->items + i;

    serialize_str(macro->name_id, &data, &data_size, size);

    reserve_space(sizeof(u32), &data, &data_size, size);
    *(u32 *) (data + *size) = macro->arg_names.len;
    *size += sizeof(u32);

    for (u32 j = 0; j < macro->arg_names.len; ++j)
      serialize_str(macro->arg_names.items[j], &data, &data_size, size);

    IndicesMap map = {0};
    serialize_ast(&macro->body, &data, &data_size, size,
                  &path_offsets, included_files->items[0], &map);

    reserve_space(sizeof(u8), &data, &data_size, size);
    *(u8 *) (data + *size) = macro->has_unpack;
    *size += sizeof(u8);
  }

  *(u32 *) data = *(u32 *) "ABM\0";
  *(u32 *) (data + sizeof(u32)) = *size;

  if (path_offsets.items)
    free(path_offsets.items);

  return data;
}
