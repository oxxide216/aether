#include "aether/serializer.h"
#include "aether/optimizer.h"

static void save_block_data(Ir *ir, u8 **data, u32 *data_size,
                            u32 *end, FilePathOffsets *path_offsets);

static void reserve_space(u32 amount, u8 **data, u32 *data_size, u32 *end) {
  u32 new_data_size = *data_size;
  while (*end + amount > new_data_size)
    new_data_size *= 2;

  if (new_data_size != *data_size) {
    *data_size = new_data_size;
    *data = realloc(*data, *data_size);
  }
}

static void save_str_data(Str str, u8 **data, u32 *data_size, u32 *end) {
  reserve_space(sizeof(u32) + str.len, data, data_size, end);
  *(u32 *) (*data + *end) = str.len;
  *end += sizeof(u32);

  for (u32 i = 0; i < str.len; ++i) {
    *(char *) (*data + *end) = str.ptr[i];
    *end += sizeof(char);
  }
}

static void save_expr_data(IrExpr *expr, u8 **data, u32 *data_size,
                           u32 *end, FilePathOffsets *path_offsets) {
  reserve_space(sizeof(u8), data, data_size, end);
  *(u8 *) (*data + *end) = expr->kind;
  *end += sizeof(u8);

  switch (expr->kind) {
  case IrExprKindBlock: {
    save_block_data(&expr->as.block.block, data, data_size, end, path_offsets);

    for (u32 i = 0; i < path_offsets->len; ++i) {
      if (str_eq(path_offsets->items[i].path, expr->as.block.file_path)) {
        reserve_space(sizeof(u32), data, data_size, end);
        *(u32 *) (*data + *end) = path_offsets->items[i].offset;
        *end += sizeof(u32);

        break;
      }
    }
  } break;

  case IrExprKindFuncCall: {
    save_expr_data(expr->as.func_call.func, data, data_size, end, path_offsets);
    save_block_data(&expr->as.func_call.args, data, data_size, end, path_offsets);
  } break;

  case IrExprKindVarDef: {
    save_str_data(expr->as.var_def.name, data, data_size, end);
    save_expr_data(expr->as.var_def.expr, data, data_size, end, path_offsets);
  } break;

  case IrExprKindIf: {
    save_expr_data(expr->as._if.cond, data, data_size, end, path_offsets);
    save_block_data(&expr->as._if.if_body, data, data_size, end, path_offsets);

    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as._if.elifs.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      save_expr_data(expr->as._if.elifs.items[i].cond, data, data_size, end, path_offsets);
      save_block_data(&expr->as._if.elifs.items[i].body, data, data_size, end, path_offsets);
    }

    reserve_space(sizeof(u8), data, data_size, end);
    *(u8 *) (*data + *end) = expr->as._if.has_else;
    *end += sizeof(u8);

    if (expr->as._if.has_else)
      save_block_data(&expr->as._if.else_body, data, data_size, end, path_offsets);
  } break;

  case IrExprKindWhile: {
    save_expr_data(expr->as._while.cond, data, data_size, end, path_offsets);
    save_block_data(&expr->as._while.body, data, data_size, end, path_offsets);
  } break;

  case IrExprKindSet: {
    save_str_data(expr->as.set.dest, data, data_size, end);
    save_expr_data(expr->as.set.src, data, data_size, end, path_offsets);
  } break;

  case IrExprKindGetAt: {
    save_expr_data(expr->as.get_at.src, data, data_size, end, path_offsets);
    save_expr_data(expr->as.get_at.key, data, data_size, end, path_offsets);
  } break;

  case IrExprKindSetAt: {
    save_str_data(expr->as.set_at.dest, data, data_size, end);
    save_expr_data(expr->as.set_at.key, data, data_size, end, path_offsets);
    save_expr_data(expr->as.set_at.value, data, data_size, end, path_offsets);
  } break;

  case IrExprKindRet: {
    reserve_space(sizeof(u8), data, data_size, end);
    *(u8 *) (*data + *end) = expr->as.ret.has_expr;
    *end += sizeof(u8);

    if (expr->as.ret.has_expr)
      save_expr_data(expr->as.ret.expr, data, data_size, end, path_offsets);
  } break;

  case IrExprKindList: {
    save_block_data(&expr->as.list, data, data_size, end, path_offsets);
  } break;

  case IrExprKindIdent: {
    save_str_data(expr->as.ident, data, data_size, end);
  } break;

  case IrExprKindString: {
    save_str_data(expr->as.string, data, data_size, end);
  } break;

  case IrExprKindInt: {
    reserve_space(sizeof(i64), data, data_size, end);
    *(i64 *) (*data + *end) = expr->as._int;
    *end += sizeof(i64);
  } break;

  case IrExprKindFloat: {
    reserve_space(sizeof(f64), data, data_size, end);
    *(f64 *) (*data + *end) = expr->as._float;
    *end += sizeof(f64);
  } break;

  case IrExprKindBool: {
    reserve_space(sizeof(u8), data, data_size, end);
    *(u8 *) (*data + *end) = expr->as._bool;
    *end += sizeof(u8);
  } break;

  case IrExprKindLambda: {
    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as.lambda.args.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      save_str_data(expr->as.lambda.args.items[i], data, data_size, end);

    save_block_data(&expr->as.lambda.body, data, data_size, end, path_offsets);
    save_str_data(expr->as.lambda.intrinsic_name, data, data_size, end);
  } break;

  case IrExprKindDict: {
    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as.dict.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      save_expr_data(expr->as.dict.items[i].key, data, data_size, end, path_offsets);
      save_expr_data(expr->as.dict.items[i].expr, data, data_size, end, path_offsets);
    }
  } break;

  case IrExprKindMatch: {
    save_expr_data(expr->as.match.src, data, data_size, end, path_offsets);

    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as.match.cases.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      save_expr_data(expr->as.match.cases.items[i].pattern, data, data_size, end, path_offsets);
      save_expr_data(expr->as.match.cases.items[i].expr, data, data_size, end, path_offsets);
    }
  } break;

  case IrExprKindSelf: break;
  }

  reserve_space(2 * sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = expr->meta.row;
  *end += sizeof(u32);
  *(u32 *) (*data + *end) = expr->meta.col;
  *end += sizeof(u32);
}

static void save_block_data(IrBlock *block, u8 **data, u32 *data_size,
                            u32 *end, FilePathOffsets *path_offsets) {
  u32 offset = *end;

  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + offset) = 0;
  *end += sizeof(u32);

  for (u32 i = 0; i < block->len; ++i) {
    if (!block->items[i]->is_dead) {
      save_expr_data(block->items[i], data, data_size, end, path_offsets);
      ++*(u32 *) (*data + offset);
    }
  }
}

static void save_included_files(u8 **data, u32 *data_size, u32 *end,
                                FilePaths *included_files,
                                FilePathOffsets *path_offsets) {
  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = included_files->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < included_files->len; ++i) {
    FilePathOffset path_offset = { included_files->items[i], *end };
    DA_APPEND(*path_offsets, path_offset);

    save_str_data(included_files->items[i], data, data_size, end);
  }
}

u8 *serialize(Ir *ir, u32 *size, FilePaths *included_files, bool dce) {
  *size = sizeof(u32) * 2;
  u32 data_size = sizeof(u32) * 2;
  u8 *data = malloc(data_size);

  if (dce)
    eliminate_dead_code(ir);

  FilePathOffsets path_offsets = {0};
  save_included_files(&data, &data_size, size, included_files, &path_offsets);
  save_block_data(ir, &data, &data_size, size, &path_offsets);

  if (path_offsets.items)
    free(path_offsets.items);

  *(u32 *) data = *(u32 *) "ABC\0";
  *(u32 *) (data + sizeof(u32)) = *size;

  return data;
}

u8 *serialize_macros(Macros *macros, u32 *size,
                     FilePaths *included_files, bool dce) {
  *size = sizeof(u32) * 2;
  u32 data_size = sizeof(u32) * 2;
  u8 *data = malloc(data_size);

  FilePathOffsets path_offsets = {0};
  save_included_files(&data, &data_size, size, included_files, &path_offsets);

  reserve_space(sizeof(u32) + macros->len, &data, &data_size, size);
  *(u32 *) (data + *size) = macros->len;
  *size += sizeof(u32);

  for (u32 i = 0; i < macros->len; ++i) {
    Macro *macro = macros->items + i;

    save_str_data(macro->name, &data, &data_size, size);

    reserve_space(sizeof(u32), &data, &data_size, size);
    *(u32 *) (data + *size) = macro->arg_names.len;
    *size += sizeof(u32);

    for (u32 i = 0; i < macro->arg_names.len; ++i)
      save_str_data(macro->arg_names.items[i], &data, &data_size, size);

    if (dce)
      eliminate_dead_code(&macro->body);

    save_block_data(&macro->body, &data, &data_size, size, &path_offsets);

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
