#include "aether/optimizer.h"

typedef struct {
  Str     name;
  IrExpr *expr;
} Def;

typedef Da(Def) Defs;

void eliminate_dead_code_block(IrBlock *block, Defs *defs);

void eliminate_dead_code_expr(IrExpr *expr, Defs *defs) {
  expr->is_dead = expr->kind == IrExprKindVarDef;

  switch (expr->kind) {
  case IrExprKindBlock: {
    eliminate_dead_code_block(&expr->as.block, defs);
  } break;

  case IrExprKindFuncCall: {
    eliminate_dead_code_expr(expr->as.func_call.func, defs);
    eliminate_dead_code_block(&expr->as.func_call.args, defs);
  } break;

  case IrExprKindVarDef: {
    Def def = { expr->as.var_def.name, expr };
    DA_APPEND(*defs, def);
  } break;

  case IrExprKindIf: {
    eliminate_dead_code_expr(expr->as._if.cond, defs);
    eliminate_dead_code_block(&expr->as._if.if_body, defs);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      eliminate_dead_code_expr(expr->as._if.elifs.items[i].cond, defs);
      eliminate_dead_code_block(&expr->as._if.elifs.items[i].body, defs);
    }

    eliminate_dead_code_block(&expr->as._if.else_body, defs);
  } break;

  case IrExprKindWhile: {
    eliminate_dead_code_expr(expr->as._while.cond, defs);
    eliminate_dead_code_block(&expr->as._while.body, defs);
  } break;

  case IrExprKindSet: {
    eliminate_dead_code_expr(expr->as.set.src, defs);
  } break;

  case IrExprKindGetAt: {
    eliminate_dead_code_expr(expr->as.get_at.src, defs);
    eliminate_dead_code_expr(expr->as.get_at.key, defs);
  } break;

  case IrExprKindSetAt: {
    eliminate_dead_code_expr(expr->as.set_at.key, defs);
    eliminate_dead_code_expr(expr->as.set_at.value, defs);
  } break;

  case IrExprKindList: {
    eliminate_dead_code_block(&expr->as.list, defs);
  } break;

  case IrExprKindIdent: {
    for (u32 i = defs->len; i > 0; --i) {
      Def *def = defs->items + i - 1;

      if (str_eq(def->name, expr->as.ident)) {
        if (def->expr->is_dead) {
          def->expr->is_dead = false;
          if (def->expr->as.var_def.expr->kind == IrExprKindLambda)
            eliminate_dead_code_block(&def->expr->as.var_def.expr->as.lambda.body, defs);
          else
            eliminate_dead_code_expr(def->expr->as.var_def.expr, defs);
        }

        break;
      }
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt: break;
  case IrExprKindFloat: break;
  case IrExprKindBool: break;

  case IrExprKindLambda: {
    eliminate_dead_code_block(&expr->as.lambda.body, defs);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      eliminate_dead_code_expr(expr->as.dict.items[i].key, defs);
      eliminate_dead_code_expr(expr->as.dict.items[i].expr, defs);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      eliminate_dead_code_expr(expr->as.ret.expr, defs);
  } break;

  case IrExprKindMatch: {
    eliminate_dead_code_expr(expr->as.match.src, defs);

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      eliminate_dead_code_expr(expr->as.match.cases.items[i].pattern, defs);
      eliminate_dead_code_expr(expr->as.match.cases.items[i].expr, defs);
    }

    if (expr->as.match.any)
      eliminate_dead_code_expr(expr->as.match.any, defs);
  } break;

  case IrExprKindSelf: break;
  }
}

void eliminate_dead_code_block(IrBlock *block, Defs *defs) {
  for (u32 i = 0; i < block->len; ++i)
    eliminate_dead_code_expr(block->items[i], defs);
}

void eliminate_dead_code(Ir *ir) {
  Defs defs = {0};

  eliminate_dead_code_block(ir, &defs);

  if (defs.items)
    free(defs.items);
}
