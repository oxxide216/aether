#include "aether/deserializer.h"
#include "shl/shl-log.h"
#include "shl/shl-arena.h"

static void get_in_block_data_size(u8 *data, u32 *size);

static void get_in_str_data_size(u8 *data, u32 *size) {
  u32 len = *(u32 *) (data + *size);
  *size += sizeof(u32) + len * sizeof(char);
}

static void get_in_expr_data_size(u8 *data, u32 *size) {
  IrExprKind kind = *(IrExprKind *) (data + *size);
  *size += sizeof(IrExprKind);

  switch (kind) {
  case IrExprKindBlock: {
    get_in_block_data_size(data, size);
  } break;

  case IrExprKindFuncCall: {
    get_in_expr_data_size(data, size);
    get_in_block_data_size(data, size);
  } break;

  case IrExprKindVarDef: {
    get_in_str_data_size(data, size);
    get_in_expr_data_size(data, size);
  } break;

  case IrExprKindIf: {
    get_in_expr_data_size(data, size);
    get_in_block_data_size(data, size);

    u32 elifs_len = *(u32 *) (data + *size);
    *size += sizeof(u32);

    for (u32 i = 0; i < elifs_len; ++i) {
      get_in_expr_data_size(data, size);
      get_in_block_data_size(data, size);
    }

    bool has_else = *(bool *) (data + *size);
    *size += sizeof(bool);

    if (has_else)
      get_in_block_data_size(data, size);
  } break;

  case IrExprKindWhile: {
    get_in_expr_data_size(data, size);
    get_in_block_data_size(data, size);
  } break;

  case IrExprKindSet: {
    get_in_expr_data_size(data, size);
    get_in_expr_data_size(data, size);
  } break;

  case IrExprKindRet: {
    bool has_expr = *(bool *) (data + *size);
    *size += sizeof(bool);

    if (has_expr)
      get_in_expr_data_size(data, size);
  } break;

  case IrExprKindList: {
    get_in_block_data_size(data, size);
  } break;

  case IrExprKindIdent:
  case IrExprKindString: {
    get_in_str_data_size(data, size);
  } break;

  case IrExprKindInt: {
    *size += sizeof(i64);
  } break;

  case IrExprKindFloat: {
    *size += sizeof(f64);
  } break;

  case IrExprKindBool: {
    *size += sizeof(bool);
  } break;

  case IrExprKindLambda: {
    u32 len = *(u32 *) (data + *size);
    *size += sizeof(u32) + len * sizeof(Str);

    get_in_block_data_size(data, size);
    get_in_str_data_size(data, size);
  } break;

  case IrExprKindDict: {
    u32 len = *(u32 *) (data + *size);
    *size += sizeof(u32);

    for (u32 i = 0; i < len; ++i) {
      get_in_str_data_size(data, size);
      get_in_expr_data_size(data, size);
    }

    get_in_str_data_size(data, size);
  } break;

  case IrExprKindSelfCall: {
    get_in_block_data_size(data, size);
  } break;
  }
}

static void get_in_block_data_size(u8 *data, u32 *size) {
  u32 len = *(u32 *) (data + *size);
  *size += sizeof(u32);

  for (u32 i = 0; i < len; ++i)
    get_in_expr_data_size(data, size);
}

static void load_block_data(IrBlock *block, u8 *data, u32 *end, RcArena *rc_arena);

static void load_str_data(Str *str, u8 *data, u32 *end, RcArena *rc_arena) {
  str->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  str->ptr = rc_arena_alloc(rc_arena, str->len * sizeof(char));
  for (u32 i = 0; i < str->len; ++i) {
    str->ptr[i] = *(char *) (data + *end);
    *end += sizeof(char);
  }
}

static void load_expr_data(IrExpr *expr, u8 *data, u32 *end, RcArena *rc_arena) {
  expr->kind = *(IrExprKind *) (data + *end);
  *end += sizeof(IrExprKind);

  switch (expr->kind) {
  case IrExprKindBlock: {
    load_block_data(&expr->as.block, data, end, rc_arena);
  } break;

  case IrExprKindFuncCall: {
    expr->as.func_call.func = aalloc(sizeof(IrExpr));

    load_expr_data(expr->as.func_call.func, data, end, rc_arena);
    load_block_data(&expr->as.func_call.args, data, end, rc_arena);
  } break;

  case IrExprKindVarDef: {
    expr->as.var_def.expr = aalloc(sizeof(IrExpr));

    load_str_data(&expr->as.var_def.name, data, end, rc_arena);
    load_expr_data(expr->as.var_def.expr, data, end, rc_arena);
  } break;

  case IrExprKindIf: {
    expr->as._if.cond = aalloc(sizeof(IrExpr));

    load_expr_data(expr->as._if.cond, data, end, rc_arena);
    load_block_data(&expr->as._if.if_body, data, end, rc_arena);

    u32 elifs_len = *(u32 *) (data + *end);
    *end += sizeof(u32);

    for (u32 i = 0; i < elifs_len; ++i) {
      IrElif elif = {0};
      elif.cond = aalloc(sizeof(IrExpr));

      load_expr_data(elif.cond, data, end, rc_arena);
      load_block_data(&elif.body, data, end, rc_arena);

      DA_APPEND(expr->as._if.elifs, elif);
    }

    expr->as._if.has_else = *(bool *) (data + *end);
    *end += sizeof(bool);

    if (expr->as._if.has_else)
      load_block_data(&expr->as._if.else_body, data, end, rc_arena);
  } break;

  case IrExprKindWhile: {
    expr->as._while.cond = aalloc(sizeof(IrExpr));

    load_expr_data(expr->as._while.cond, data, end, rc_arena);
    load_block_data(&expr->as._while.body, data, end, rc_arena);
  } break;

  case IrExprKindSet: {
    expr->as.set.dest = aalloc(sizeof(IrExpr));
    expr->as.set.src = aalloc(sizeof(IrExpr));

    load_expr_data(expr->as.set.dest, data, end, rc_arena);
    load_expr_data(expr->as.set.src, data, end, rc_arena);
  } break;

  case IrExprKindRet: {
    expr->as.ret.has_expr = *(bool *) (data + *end);
    *end += sizeof(bool);

    if (expr->as.ret.has_expr) {
      expr->as.ret.expr = aalloc(sizeof(IrExpr));

      load_expr_data(expr->as.ret.expr, data, end, rc_arena);
    }
  } break;

  case IrExprKindList: {
    load_block_data(&expr->as.list.content, data, end, rc_arena);
  } break;

  case IrExprKindIdent: {
    load_str_data(&expr->as.ident.ident, data, end, rc_arena);
  } break;

  case IrExprKindString: {
    load_str_data(&expr->as.string.lit, data, end, rc_arena);
  } break;

  case IrExprKindInt: {
    expr->as._int._int = *(i64 *) (data + *end);
    *end += sizeof(i64);
  } break;

  case IrExprKindFloat: {
    expr->as._float._float = *(f64 *) (data + *end);
    *end += sizeof(f64);
  } break;

  case IrExprKindBool: {
    expr->as._bool._bool = *(bool *) (data + *end);
    *end += sizeof(bool);
  } break;

  case IrExprKindLambda: {
    IrArgs *args = &expr->as.lambda.args;

    args->len = *(u32 *) (data + *end);
    args->cap = args->len;
    *end += sizeof(u32);

    args->items = malloc(args->len * sizeof(Str));
    for (u32 i = 0; i < args->len; ++i)
      load_str_data(args->items + i, data, end, rc_arena);

    load_block_data(&expr->as.lambda.body, data, end, rc_arena);
    load_str_data(&expr->as.lambda.intrinsic_name, data, end, rc_arena);
  } break;

  case IrExprKindDict: {
    expr->as.dict.len = *(u32 *) (data + *end);
    *end += sizeof(u32);

    expr->as.dict.items = malloc(expr->as.dict.len * sizeof(IrField));
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      expr->as.dict.items[i].key = aalloc(sizeof(IrExpr));
      expr->as.dict.items[i].expr = aalloc(sizeof(IrExpr));

      load_expr_data(expr->as.dict.items[i].key, data, end, rc_arena);
      load_expr_data(expr->as.dict.items[i].expr, data, end, rc_arena);
    }
  } break;

  case IrExprKindSelfCall: {
    load_block_data(&expr->as.self_call.args, data, end, rc_arena);
  } break;
  }
}

static void load_block_data(IrBlock *block, u8 *data, u32 *end, RcArena *rc_arena) {
  block->len = *(u32 *) (data + *end);
  block->cap = block->len;
  *end += sizeof(u32);

  block->items = malloc(block->len * sizeof(IrExpr *));
  for (u32 i = 0; i < block->len; ++i) {
    block->items[i] = aalloc(sizeof(IrExpr));
    load_expr_data(block->items[i], data, end, rc_arena);
  }
}

Ir deserialize(u8 *data, u32 size, RcArena *rc_arena) {
  Ir ir = {0};

  if (size < sizeof(u32)) {
    ERROR("Corrupted bytecode: not enough data\n");
    exit(1);
  }

  u32 expected_size = *(u32 *) data;
  if (size != expected_size) {
    ERROR("Corrupted bytecode: expected %u, but got %u bytes\n",
          expected_size, size);
    exit(1);
  }

  u32 end = sizeof(u32);
  load_block_data(&ir, data, &end, rc_arena);

  return ir;
}
