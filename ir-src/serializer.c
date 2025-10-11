#include "aether-ir/serializer.h"

#define DATA_SIZE_DELTA 64

static void save_block_data(Ir *ir, u8 **data, u32 *data_size, u32 *end);

static void reserve_space(u32 amount, u8 **data, u32 *data_size, u32 *end) {
  u32 new_data_size = *data_size;
  while (*end + amount > new_data_size)
    new_data_size += DATA_SIZE_DELTA;

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

static void save_expr_data(IrExpr *expr, u8 **data, u32 *data_size, u32 *end) {
  reserve_space(sizeof(IrExprKind), data, data_size, end);
  *(IrExprKind *) (*data + *end) = expr->kind;
  *end += sizeof(IrExprKind);

  switch (expr->kind) {
  case IrExprKindBlock: {
    save_block_data(&expr->as.block, data, data_size, end);
  } break;

  case IrExprKindFuncCall: {
    save_expr_data(expr->as.func_call.func, data, data_size, end);
    save_block_data(&expr->as.func_call.args, data, data_size, end);
  } break;

  case IrExprKindVarDef: {
    save_str_data(expr->as.var_def.name, data, data_size, end);
    save_expr_data(expr->as.var_def.expr, data, data_size, end);
  } break;

  case IrExprKindIf: {
    save_expr_data(expr->as._if.cond, data, data_size, end);
    save_block_data(&expr->as._if.if_body, data, data_size, end);

    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as._if.elifs.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      save_expr_data(expr->as._if.elifs.items[i].cond, data, data_size, end);
      save_block_data(&expr->as._if.elifs.items[i].body, data, data_size, end);
    }

    reserve_space(sizeof(bool), data, data_size, end);
    *(bool *) (*data + *end) = expr->as._if.has_else;
    *end += sizeof(bool);

    if (expr->as._if.has_else)
      save_block_data(&expr->as._if.else_body, data, data_size, end);
  } break;

  case IrExprKindWhile: {
    save_expr_data(expr->as._while.cond, data, data_size, end);
    save_block_data(&expr->as._while.body, data, data_size, end);
  } break;

  case IrExprKindSet: {
    save_str_data(expr->as.set.dest, data, data_size, end);
    save_expr_data(expr->as.set.src, data, data_size, end);
  } break;

  case IrExprKindField: {
    save_expr_data(expr->as.field.record, data, data_size, end);
    save_str_data(expr->as.field.field, data, data_size, end);

    reserve_space(sizeof(bool), data, data_size, end);
    *(bool *) (*data + *end) = expr->as.field.is_set;
    *end += sizeof(bool);

    if (expr->as.field.is_set)
      save_expr_data(expr->as.field.expr, data, data_size, end);
  } break;

  case IrExprKindRet: {
    reserve_space(sizeof(bool), data, data_size, end);
    *(bool *) (*data + *end) = expr->as.ret.has_expr;
    *end += sizeof(bool);

    if (expr->as.ret.has_expr)
      save_expr_data(expr->as.ret.expr, data, data_size, end);
  } break;

  case IrExprKindList: {
    save_block_data(&expr->as.list.content, data, data_size, end);
  } break;

  case IrExprKindIdent: {
    save_str_data(expr->as.ident.ident, data, data_size, end);
  } break;

  case IrExprKindString: {
    save_str_data(expr->as.string.lit, data, data_size, end);
  } break;

  case IrExprKindInt: {
    reserve_space(sizeof(i64), data, data_size, end);
    *(i64 *) (*data + *end) = expr->as._int._int;
    *end += sizeof(i64);
  } break;

  case IrExprKindFloat: {
    reserve_space(sizeof(f64), data, data_size, end);
    *(f64 *) (*data + *end) = expr->as._float._float;
    *end += sizeof(f64);
  } break;

  case IrExprKindBool: {
    reserve_space(sizeof(bool), data, data_size, end);
    *(bool *) (*data + *end) = expr->as._bool._bool;
    *end += sizeof(bool);
  } break;

  case IrExprKindLambda: {
    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as.lambda.args.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as.lambda.args.len; ++i)
      save_str_data(expr->as.lambda.args.items[i], data, data_size, end);

    save_block_data(&expr->as.lambda.body, data, data_size, end);
    save_str_data(expr->as.lambda.intrinsic_name, data, data_size, end);
  } break;

  case IrExprKindRecord: {
    reserve_space(sizeof(u32), data, data_size, end);
    *(u32 *) (*data + *end) = expr->as.record.len;
    *end += sizeof(u32);

    for (u32 i = 0; i < expr->as.record.len; ++i) {
      save_str_data(expr->as.record.items[i].name, data, data_size, end);
      save_expr_data(expr->as.record.items[i].expr, data, data_size, end);
    }
  } break;
  }
}

static void save_block_data(IrBlock *block, u8 **data, u32 *data_size, u32 *end) {
  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = block->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < block->len; ++i)
    save_expr_data(block->items[i], data, data_size, end);
}

u8 *serialize(Ir *ir, u32 *size) {
  *size = sizeof(u32);
  u32 data_size = sizeof(u32);
  u8 *data = malloc(data_size);

  save_block_data(ir, &data, &data_size, size);

  *(u32 *) data = *size;

  return data;
}
