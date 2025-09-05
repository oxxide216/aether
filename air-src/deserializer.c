#include "deserializer.h"
#include "shl_log.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

static void get_block_data_size(u8 *data, u32 *size);

static void get_str_data_size(u8 *data, u32 *size) {
  u32 len = *(u32 *) (data + *size);
  *size += sizeof(u32) + len * sizeof(char);
}

static void get_expr_data_size(u8 *data, u32 *size) {
  IrExprKind kind = *(IrExprKind *) (data + *size);
  *size += sizeof(IrExprKind);

  switch (kind) {
  case IrExprKindFuncDef: {
    get_str_data_size(data, size);

    u32 len = *(u32 *) (data + *size);
    *size += sizeof(u32) + len * sizeof(Str);

    get_block_data_size(data, size);
  } break;

  case IrExprKindFuncCall: {
    get_str_data_size(data, size);
    get_block_data_size(data, size);
  } break;

  case IrExprKindList: {
    get_block_data_size(data, size);
  } break;

  case IrExprKindIdent:
  case IrExprKindStrLit: {
    get_str_data_size(data, size);
  } break;

  case IrExprKindNumber: {
    *size += sizeof(i64);
  } break;

  case IrExprKindVarDef: {
    get_str_data_size(data, size);
    get_expr_data_size(data, size);
  } break;
  }
}

static void get_block_data_size(u8 *data, u32 *size) {
  u32 len = *(u32 *) (data + *size);
  *size += sizeof(u32);

  for (u32 i = 0; i < len; ++i)
    get_expr_data_size(data, size);
}

static void load_block_data(IrBlock *block, u8 *data, u32 *end);

static void load_str_data(Str *str, u8 *data, u32 *end) {
  str->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  str->ptr = malloc(str->len * sizeof(char));
  for (u32 i = 0; i < str->len; ++i) {
    str->ptr[i] = *(char *) (data + *end);
    *end += sizeof(char);
  }
}

static void load_expr_data(IrExpr *expr, u8 *data, u32 *end) {
  expr->kind = *(IrExprKind *) (data + *end);
  *end += sizeof(IrExprKind);

  switch (expr->kind) {
  case IrExprKindFuncDef: {
    load_str_data(&expr->as.func_def.name, data, end);

    IrArgs *args = &expr->as.func_def.args;

    args->len = *(u32 *) (data + *end);
    args->cap = args->len;
    *end += sizeof(u32);

    args->items = malloc(args->len * sizeof(Str));
    for (u32 i = 0; i < args->len; ++i)
      load_str_data(args->items + i, data, end);

    load_block_data(&expr->as.func_def.body, data, end);
  } break;

  case IrExprKindFuncCall: {
    load_str_data(&expr->as.func_call.name, data, end);
    load_block_data(&expr->as.func_call.args, data, end);
  } break;

  case IrExprKindList: {
    load_block_data(&expr->as.list.content, data, end);
  } break;

  case IrExprKindIdent: {
    load_str_data(&expr->as.ident.ident, data, end);
  } break;

  case IrExprKindStrLit: {
    load_str_data(&expr->as.str_lit.lit, data, end);
  } break;

  case IrExprKindNumber: {
    expr->as.number.number = *(i64 *) (data + *end);
    *end += sizeof(i64);
  } break;

  case IrExprKindVarDef: {
    expr->as.var_def.expr = aalloc(sizeof(IrExpr));

    load_str_data(&expr->as.str_lit.lit, data, end);
    load_expr_data(expr->as.var_def.expr, data, end);
  } break;
  }
}

static void load_block_data(IrBlock *block, u8 *data, u32 *end) {
  block->len = *(u32 *) (data + *end);
  block->cap = block->len;
  *end += sizeof(u32);

  block->items = malloc(block->len * sizeof(IrExpr *));
  for (u32 i = 0; i < block->len; ++i) {
    block->items[i] = aalloc(sizeof(IrExpr));
    load_expr_data(block->items[i], data, end);
  }
}

Ir deserialize(u8 *data, u32 size) {
  Ir ir = {0};

  u32 real_size = *(u32 *) data;
  if (real_size + sizeof(u32) != size) {
    ERROR("Corrupted bytecode: expected %u, but got %u bytes\n",
          size, real_size);
    exit(1);
  }

  u32 end = sizeof(u32);
  load_block_data(&ir, data, &end);

  return ir;
}
