#include "serializer.h"

static void get_block_size(Ir *ir, u32 *size);

static void get_expr_size(IrExpr *expr, u32 *size) {
  *size += sizeof(IrExprKind);

  switch (expr->kind) {
  case IrExprKindFuncDef: {
    *size += sizeof(u32) * 2 + expr->as.func_def.name.len * sizeof(char);
    for (u32 i = 0; i < expr->as.func_def.args.len; ++i)
      *size += sizeof(u32) + expr->as.func_def.args.items[i].len * sizeof(char);

    get_block_size(&expr->as.func_def.body, size);
  } break;

  case IrExprKindFuncCall: {
    *size += sizeof(u32) + sizeof(char *);
    get_block_size(&expr->as.func_call.args, size);
  } break;

  case IrExprKindList: {
    get_block_size(&expr->as.list.content, size);
  } break;

  case IrExprKindIdent:
  case IrExprKindStrLit: {
    *size += sizeof(u32) + sizeof(char *);
  } break;

  case IrExprKindNumber: {
    *size += sizeof(i64);
  } break;

  case IrExprKindVarDef: {
    *size += sizeof(u32) + expr->as.var_def.name.len * sizeof(char);
    get_expr_size(expr->as.var_def.expr, size);
  } break;
  }
}

static void get_block_size(Ir *ir, u32 *size) {
  *size += sizeof(u32);

  for (u32 i = 0; i < ir->len; ++i)
    get_expr_size(ir->items[i], size);
}

static void save_block_data(Ir *ir, u8 *data, u32 *end);

static void save_str_data(Str str, u8 *data, u32 *end) {
  *(u32 *) (data + *end) = str.len;
  *end += sizeof(u32);

  for (u32 i = 0; i < str.len; ++i) {
    *(char *) (data + *end) = str.ptr[i];
    *end += sizeof(char);
  }
}

static void save_expr_data(IrExpr *expr, u8 *data, u32 *end) {
  *(IrExprKind *) (data + *end) = expr->kind;
  *end += sizeof(IrExprKind);

  switch (expr->kind) {
  case IrExprKindFuncDef: {
    save_str_data(expr->as.func_def.name, data, end);

    *(u32 *) (data + *end) = expr->as.func_def.args.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as.func_def.args.len; ++i)
      save_str_data(expr->as.func_def.args.items[i], data, end);

    save_block_data(&expr->as.func_def.body, data, end);
  } break;

  case IrExprKindFuncCall: {
    save_str_data(expr->as.func_call.name, data, end);
    save_block_data(&expr->as.func_call.args, data, end);
  } break;

  case IrExprKindList: {
    save_block_data(&expr->as.list.content, data, end);
  } break;

  case IrExprKindIdent: {
    save_str_data(expr->as.ident.ident, data, end);
  } break;

  case IrExprKindStrLit: {
    save_str_data(expr->as.str_lit.lit, data, end);
  } break;

  case IrExprKindNumber: {
    *(i64 *) (data + *end) = expr->as.number.number;
    *end += sizeof(i64);
  } break;

  case IrExprKindVarDef: {
    save_str_data(expr->as.var_def.name, data, end);
    save_expr_data(expr->as.var_def.expr, data, end);
  } break;
  }
}

static void save_block_data(Ir *ir, u8 *data, u32 *end) {
  *(u32 *) (data + *end) = ir->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < ir->len; ++i)
    save_expr_data(ir->items[i], data, end);
}

u8 *serialize(Ir *ir, u32 *size) {
  get_block_size(ir, size);

  u8 *data = malloc(*size + sizeof(u32));
  *(u32 *) data = *size;
  *size += sizeof(u32);

  u32 end = sizeof(u32);
  save_block_data(ir, data, &end);

  return data;
}
