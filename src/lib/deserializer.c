#include "aether/deserializer.h"
#include "aether/serializer.h"
#include "shl/shl-log.h"

static void load_block_data(IrBlock *block, u8 *data, u32 *end,
                            FilePathOffsets *path_offsets, Arena *arena);

static void load_str_data(Str *str, u8 *data, u32 *end, Arena *arena) {
  str->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  str->ptr = arena_alloc(arena, str->len);
  for (u32 i = 0; i < str->len; ++i) {
    str->ptr[i] = *(char *) (data + *end);
    *end += 1;
  }
}

static void load_expr_data(IrExpr *expr, u8 *data, u32 *end,
                           FilePathOffsets *path_offsets, Arena *arena) {
  expr->kind = *(u8 *) (data + *end);
  *end += sizeof(u8);

  switch (expr->kind) {
  case IrExprKindBlock: {
    load_block_data(&expr->as.block.block, data, end, path_offsets, arena);

    u32 file_path_offset = *(u32 *) (data + *end);
    *end += sizeof(u32);

    load_str_data(&expr->as.block.file_path, data, &file_path_offset, arena);
  } break;

  case IrExprKindFuncCall: {
    expr->as.func_call.func = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as.func_call.func, data, end, path_offsets, arena);
    load_block_data(&expr->as.func_call.args, data, end, path_offsets, arena);
  } break;

  case IrExprKindVarDef: {
    expr->as.var_def.expr = arena_alloc(arena, sizeof(IrExpr));

    load_str_data(&expr->as.var_def.name, data, end, arena);
    load_expr_data(expr->as.var_def.expr, data, end, path_offsets, arena);
  } break;

  case IrExprKindIf: {
    expr->as._if.cond = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as._if.cond, data, end, path_offsets, arena);
    load_block_data(&expr->as._if.if_body, data, end, path_offsets, arena);

    expr->as._if.elifs.len = *(u32 *) (data + *end);
    *end += sizeof(u32);

    expr->as._if.elifs.items = arena_alloc(arena, expr->as._if.elifs.len * sizeof(IrElif));
    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      IrElif elif = {0};
      elif.cond = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(elif.cond, data, end, path_offsets, arena);
      load_block_data(&elif.body, data, end, path_offsets, arena);

      expr->as._if.elifs.items[i] = elif;
    }

    expr->as._if.has_else = *(u8 *) (data + *end);
    *end += sizeof(u8);

    if (expr->as._if.has_else)
      load_block_data(&expr->as._if.else_body, data, end, path_offsets, arena);
  } break;

  case IrExprKindWhile: {
    expr->as._while.cond = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as._while.cond, data, end, path_offsets, arena);
    load_block_data(&expr->as._while.body, data, end, path_offsets, arena);
  } break;

  case IrExprKindSet: {
    expr->as.set.src = arena_alloc(arena, sizeof(IrExpr));

    load_str_data(&expr->as.set.dest, data, end, arena);
    load_expr_data(expr->as.set.src, data, end, path_offsets, arena);
  } break;

  case IrExprKindGetAt: {
    expr->as.get_at.src = arena_alloc(arena, sizeof(IrExpr));
    expr->as.get_at.key = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as.get_at.src, data, end, path_offsets, arena);
    load_expr_data(expr->as.get_at.key, data, end, path_offsets, arena);
  } break;

  case IrExprKindSetAt: {
    expr->as.set_at.key = arena_alloc(arena, sizeof(IrExpr));
    expr->as.set_at.value = arena_alloc(arena, sizeof(IrExpr));

    load_str_data(&expr->as.set_at.dest, data, end, arena);
    load_expr_data(expr->as.set_at.key, data, end, path_offsets, arena);
    load_expr_data(expr->as.set_at.value, data, end, path_offsets, arena);
  } break;

  case IrExprKindRet: {
    expr->as.ret.has_expr = *(u8 *) (data + *end);
    *end += sizeof(u8);

    if (expr->as.ret.has_expr) {
      expr->as.ret.expr = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(expr->as.ret.expr, data, end, path_offsets, arena);
    }
  } break;

  case IrExprKindList: {
    load_block_data(&expr->as.list, data, end, path_offsets, arena);
  } break;

  case IrExprKindIdent: {
    load_str_data(&expr->as.ident, data, end, arena);
  } break;

  case IrExprKindString: {
    load_str_data(&expr->as.string, data, end, arena);
  } break;

  case IrExprKindInt: {
    expr->as._int = *(i64 *) (data + *end);
    *end += sizeof(i64);
  } break;

  case IrExprKindFloat: {
    expr->as._float = *(f64 *) (data + *end);
    *end += sizeof(f64);
  } break;

  case IrExprKindBool: {
    expr->as._bool = *(u8 *) (data + *end);
    *end += sizeof(u8);
  } break;

  case IrExprKindLambda: {
    IrArgs *args = &expr->as.lambda.args;

    args->len = *(u32 *) (data + *end);
    *end += sizeof(u32);

    args->items = arena_alloc(arena, args->len * sizeof(Str));
    for (u32 i = 0; i < args->len; ++i)
      load_str_data(args->items + i, data, end, arena);

    load_block_data(&expr->as.lambda.body, data, end, path_offsets, arena);
    load_str_data(&expr->as.lambda.intrinsic_name, data, end, arena);
  } break;

  case IrExprKindDict: {
    expr->as.dict.len = *(u32 *) (data + *end);
    *end += sizeof(u32);

    expr->as.dict.items = arena_alloc(arena, expr->as.dict.len * sizeof(IrField));
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      expr->as.dict.items[i].key = arena_alloc(arena, sizeof(IrExpr));
      expr->as.dict.items[i].expr = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(expr->as.dict.items[i].key, data, end, path_offsets, arena);
      load_expr_data(expr->as.dict.items[i].expr, data, end, path_offsets, arena);
    }
  } break;

  case IrExprKindMatch: {
    expr->as.match.src = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as.match.src, data, end, path_offsets, arena);

    expr->as.match.cases.len = *(u32 *) (data + *end);
    *end += sizeof(u32);

    expr->as.match.cases.items = arena_alloc(arena, expr->as.match.cases.len * sizeof(IrCase));
    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      expr->as.match.cases.items[i].pattern = arena_alloc(arena, sizeof(IrExpr));
      expr->as.match.cases.items[i].expr = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(expr->as.match.cases.items[i].pattern, data,
                     end, path_offsets, arena);
      load_expr_data(expr->as.match.cases.items[i].expr, data,
                     end, path_offsets, arena);
    }
  } break;

  case IrExprKindSelf: break;

  default: {
    ERROR("Corrupted bytecode: unknown expression kind\n");
    exit(1);
  } break;
  }

  expr->meta.row = *(u32 *) (data + *end);
  *end += sizeof(u32);
  expr->meta.col = *(u32 *) (data + *end);
  *end += sizeof(u32);
}

static void load_block_data(IrBlock *block, u8 *data, u32 *end,
                            FilePathOffsets *path_offsets, Arena *arena) {
  block->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  block->items = arena_alloc(arena, block->len * sizeof(IrExpr *));

  for (u32 i = 0; i < block->len; ++i) {
    block->items[i] = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(block->items[i], data, end, path_offsets, arena);
  }
}

static void load_path_offsets_data(FilePathOffsets *path_offsets,
                                   u8 *data, u32 *end, Arena *arena) {
  path_offsets->len = *(u32 *) (data + *end);
  path_offsets->cap = path_offsets->len;
  *end += sizeof(u32);

  path_offsets->items = arena_alloc(arena, path_offsets->cap * sizeof(FilePathOffset));
  for (u32 i = 0; i < path_offsets->len; ++i) {
    path_offsets->items[i].offset = *end;
    load_str_data(&path_offsets->items[i].path, data, end, arena);
  }
}

Ir deserialize(u8 *data, u32 size, Arena *arena, Str *file_path) {
  Ir ir = {0};

  if (size < sizeof(u32) * 2) {
    ERROR("Corrupted bytecode: not enough data\n");
    exit(1);
  }

  if (!str_eq(STR((char *) data, 4), STR_LIT("ABC\0"))) {
    ERROR("Corrupted bytecode: wrong magic\n");
    exit(1);
  }

  u32 expected_size = *(u32 *) (data + sizeof(u32));
  if (size != expected_size) {
    ERROR("Corrupted bytecode: expected %u, but got %u bytes\n",
          expected_size, size);
    exit(1);
  }

  u32 end = sizeof(u32) * 2;

  FilePathOffsets path_offsets = {0};
  load_path_offsets_data(&path_offsets, data, &end, arena);

  *file_path = path_offsets.items[0].path;

  load_block_data(&ir, data, &end, &path_offsets, arena);

  return ir;
}

Macros deserialize_macros(u8 *data, u32 size, Arena *arena) {
  Macros macros = {0};

  if (size < sizeof(u32)) {
    ERROR("Corrupted bytecode: not enough data\n");
    exit(1);
  }

  if (!str_eq(STR((char *) data, 4), STR_LIT("ABM\0"))) {
    ERROR("Corrupted bytecode: wrong magic\n");
    exit(1);
  }

  u32 expected_size = *(u32 *) (data + sizeof(u32));
  if (size != expected_size) {
    ERROR("Corrupted bytecode: expected %u, but got %u bytes\n",
          expected_size, size);
    exit(1);
  }

  u32 end = sizeof(u32) * 2;

  FilePathOffsets path_offsets = {0};
  load_path_offsets_data(&path_offsets, data, &end, arena);

  macros.len = *(u32 *) (data + end);
  macros.cap = macros.len;
  end += sizeof(u32);

  macros.items = arena_alloc(arena, macros.cap * sizeof(Macro));
  for (u32 i = 0; i < macros.len; ++i) {
    Macro *macro = macros.items + i;

    load_str_data(&macro->name, data, &end, arena);

    macro->arg_names.len = *(u32 *) (data + end);
    end += sizeof(u32);

    macro->arg_names.items = arena_alloc(arena, macro->arg_names.len * sizeof(Str));
    for (u32 j = 0; j < macro->arg_names.len; ++j)
      load_str_data(macro->arg_names.items + j, data, &end, arena);

    load_block_data(&macro->body, data, &end, &path_offsets, arena);

    macro->has_unpack = *(u8 *) (data + end);
    end += sizeof(u8);
  }

  return macros;
}
