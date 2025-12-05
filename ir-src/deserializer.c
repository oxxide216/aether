#include "aether/deserializer.h"
#include "shl/shl-log.h"

static void load_block_data(IrBlock *block, u8 *data, u32 *end,
                            Arena *arena, Arena *persistent_arena);

static void load_str_data(Str *str, u8 *data, u32 *end, Arena *persistent_arena) {
  str->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  str->ptr = arena_alloc(persistent_arena, str->len);
  for (u32 i = 0; i < str->len; ++i) {
    str->ptr[i] = *(char *) (data + *end);
    *end += 1;
  }
}

static void load_expr_data(IrExpr *expr, u8 *data, u32 *end,
                           Arena *arena, Arena *persistent_arena) {
  expr->kind = *(u32 *) (data + *end);
  *end += sizeof(u32);

  switch (expr->kind) {
  case IrExprKindBlock: {
    load_block_data(&expr->as.block, data, end, arena, persistent_arena);
  } break;

  case IrExprKindFuncCall: {
    expr->as.func_call.func = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as.func_call.func, data, end, arena, persistent_arena);
    load_block_data(&expr->as.func_call.args, data, end, arena, persistent_arena);
  } break;

  case IrExprKindVarDef: {
    expr->as.var_def.expr = arena_alloc(arena, sizeof(IrExpr));

    load_str_data(&expr->as.var_def.name, data, end, persistent_arena);
    load_expr_data(expr->as.var_def.expr, data, end, arena, persistent_arena);
  } break;

  case IrExprKindIf: {
    expr->as._if.cond = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as._if.cond, data, end, arena, persistent_arena);
    load_block_data(&expr->as._if.if_body, data, end, arena, persistent_arena);

    expr->as._if.elifs.len = *(u32 *) (data + *end);
    expr->as._if.elifs.cap = expr->as._if.elifs.len;
    *end += sizeof(u32);

    expr->as._if.elifs.items = arena_alloc(arena, expr->as._if.elifs.cap * sizeof(IrElif));
    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      IrElif elif = {0};
      elif.cond = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(elif.cond, data, end, arena, persistent_arena);
      load_block_data(&elif.body, data, end, arena, persistent_arena);

      expr->as._if.elifs.items[i] = elif;
    }

    expr->as._if.has_else = *(u8 *) (data + *end);
    *end += sizeof(u8);

    if (expr->as._if.has_else)
      load_block_data(&expr->as._if.else_body, data, end, arena, persistent_arena);
  } break;

  case IrExprKindWhile: {
    expr->as._while.cond = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as._while.cond, data, end, arena, persistent_arena);
    load_block_data(&expr->as._while.body, data, end, arena, persistent_arena);
  } break;

  case IrExprKindSet: {
    expr->as.set.src = arena_alloc(arena, sizeof(IrExpr));

    load_str_data(&expr->as.set.dest, data, end, persistent_arena);
    load_expr_data(expr->as.set.src, data, end, arena, persistent_arena);
  } break;

  case IrExprKindGetAt: {
    expr->as.get_at.src = arena_alloc(arena, sizeof(IrExpr));
    expr->as.get_at.key = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as.get_at.src, data, end, arena, persistent_arena);
    load_expr_data(expr->as.get_at.key, data, end, arena, persistent_arena);
  } break;

  case IrExprKindSetAt: {
    expr->as.set_at.key = arena_alloc(arena, sizeof(IrExpr));
    expr->as.set_at.value = arena_alloc(arena, sizeof(IrExpr));

    load_str_data(&expr->as.set_at.dest, data, end, persistent_arena);
    load_expr_data(expr->as.set_at.key, data, end, arena, persistent_arena);
    load_expr_data(expr->as.set_at.value, data, end, arena, persistent_arena);
  } break;

  case IrExprKindRet: {
    expr->as.ret.has_expr = *(u8 *) (data + *end);
    *end += sizeof(u8);

    if (expr->as.ret.has_expr) {
      expr->as.ret.expr = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(expr->as.ret.expr, data, end, arena, persistent_arena);
    }
  } break;

  case IrExprKindList: {
    load_block_data(&expr->as.list.content, data, end, arena, persistent_arena);
  } break;

  case IrExprKindIdent: {
    load_str_data(&expr->as.ident.ident, data, end, persistent_arena);
  } break;

  case IrExprKindString: {
    load_str_data(&expr->as.string.lit, data, end, persistent_arena);
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
    expr->as._bool._bool = *(u8 *) (data + *end);
    *end += sizeof(u8);
  } break;

  case IrExprKindLambda: {
    IrArgs *args = &expr->as.lambda.args;

    args->len = *(u32 *) (data + *end);
    args->cap = args->len;
    *end += sizeof(u32);

    args->items = arena_alloc(persistent_arena, args->cap * sizeof(Str));
    for (u32 i = 0; i < args->len; ++i)
      load_str_data(args->items + i, data, end, persistent_arena);

    load_block_data(&expr->as.lambda.body, data, end, persistent_arena, persistent_arena);
    load_str_data(&expr->as.lambda.intrinsic_name, data, end, persistent_arena);
  } break;

  case IrExprKindDict: {
    expr->as.dict.len = *(u32 *) (data + *end);
    expr->as.dict.cap = expr->as.dict.len;
    *end += sizeof(u32);

    expr->as.dict.items = arena_alloc(arena, expr->as.dict.cap * sizeof(IrField));
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      expr->as.dict.items[i].key = arena_alloc(arena, sizeof(IrExpr));
      expr->as.dict.items[i].expr = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(expr->as.dict.items[i].key, data, end, arena, persistent_arena);
      load_expr_data(expr->as.dict.items[i].expr, data, end, arena, persistent_arena);
    }
  } break;

  case IrExprKindMatch: {
    expr->as.match.src = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(expr->as.match.src, data, end, arena, persistent_arena);

    expr->as.match.cases.len = *(u32 *) (data + *end);
    expr->as.match.cases.cap = expr->as.match.cases.len;
    *end += sizeof(u32);

    expr->as.match.cases.items = arena_alloc(arena, expr->as.match.cases.cap * sizeof(IrCase));
    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      expr->as.match.cases.items[i].pattern = arena_alloc(arena, sizeof(IrExpr));
      expr->as.match.cases.items[i].expr = arena_alloc(arena, sizeof(IrExpr));

      load_expr_data(expr->as.match.cases.items[i].pattern, data, end, arena, persistent_arena);
      load_expr_data(expr->as.match.cases.items[i].expr, data, end, arena, persistent_arena);
    }
  } break;

  case IrExprKindSelf: break;

  default: {
    ERROR("Corrupted bytecode: unknown expression kind\n");
    exit(1);
  } break;
  }

  load_str_data(&expr->meta.file_path, data, end, persistent_arena);
  expr->meta.row = *(u32 *) (data + *end);
  *end += sizeof(u32);
  expr->meta.col = *(u32 *) (data + *end);
  *end += sizeof(u32);
}

static void load_block_data(IrBlock *block, u8 *data, u32 *end,
                            Arena *arena, Arena *persistent_arena) {
  block->len = *(u32 *) (data + *end);
  block->cap = block->len;
  *end += sizeof(u32);

  block->items = arena_alloc(arena, block->cap * sizeof(IrExpr *));

  for (u32 i = 0; i < block->len; ++i) {
    block->items[i] = arena_alloc(arena, sizeof(IrExpr));

    load_expr_data(block->items[i], data, end, arena, persistent_arena);
  }
}

Ir deserialize(u8 *data, u32 size, Arena *arena, Arena *persistent_arena) {
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
  load_block_data(&ir, data, &end, arena, persistent_arena);

  return ir;
}

Macros deserialize_macros(u8 *data, u32 size, Arena *arena, Arena *persistent_arena) {
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

  macros.len = *(u32 *) (data + end);
  macros.cap = macros.len;
  end += sizeof(u32);

  macros.items = arena_alloc(persistent_arena, macros.cap * sizeof(Macro));
  for (u32 i = 0; i < macros.len; ++i) {
    Macro *macro = macros.items + i;

    load_str_data(&macro->name, data, &end, persistent_arena);

    macro->arg_names.len = *(u32 *) (data + end);
    macro->arg_names.cap = macro->arg_names.len;
    end += sizeof(u32);

    macro->arg_names.items = arena_alloc(persistent_arena, macro->arg_names.cap * sizeof(Str));
    for (u32 j = 0; j < macro->arg_names.len; ++j)
      load_str_data(macro->arg_names.items + j, data, &end, persistent_arena);

    load_block_data(&macro->body, data, &end, arena, persistent_arena);

    macro->has_unpack = *(u8 *) (data + end);
    end += sizeof(u8);
  }

  return macros;
}
