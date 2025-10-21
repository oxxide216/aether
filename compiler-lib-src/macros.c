#include "macros.h"
#include "shl_arena.h"

static void clone_block(IrBlock *block);

static void clone_expr(IrExpr **expr) {
  IrExpr *new_expr = aalloc(sizeof(IrExpr));
  *new_expr = **expr;
  *expr = new_expr;

  switch (new_expr->kind) {
  case IrExprKindBlock: {
    clone_block(&new_expr->as.block);
  } break;

  case IrExprKindFuncCall: {
    clone_expr(&new_expr->as.func_call.func);
    clone_block(&new_expr->as.func_call.args);
  } break;

  case IrExprKindVarDef: {
    clone_expr(&new_expr->as.var_def.expr);
  } break;

  case IrExprKindIf: {
    clone_expr(&new_expr->as._if.cond);
    clone_block(&new_expr->as._if.if_body);

    for (u32 i = 0; i < new_expr->as._if.elifs.len; ++i) {
      clone_expr(&new_expr->as._if.elifs.items[i].cond);
      clone_block(&new_expr->as._if.elifs.items[i].body);
    }

    if (new_expr->as._if.has_else)
      clone_block(&new_expr->as._if.else_body);
  } break;

  case IrExprKindWhile: {
    clone_expr(&new_expr->as._while.cond);
    clone_block(&new_expr->as._while.body);
  } break;

  case IrExprKindSet: {
    clone_expr(&new_expr->as.set.src);
  } break;

  case IrExprKindField: {
    clone_expr(&new_expr->as.field.record);
    clone_expr(&new_expr->as.field.expr);
  } break;

  case IrExprKindList: {
    clone_block(&new_expr->as.list.content);
  } break;

  case IrExprKindIdent:  break;
  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    clone_block(&new_expr->as.lambda.body);
  } break;

  case IrExprKindRecord: {
    for (u32 i = 0; i < new_expr->as.record.len; ++i)
      clone_expr(&new_expr->as.record.items[i].expr);
  } break;

  case IrExprKindRet: {
    clone_expr(&new_expr->as.ret.expr);
  } break;

  case IrExprKindSelfCall: {
    clone_block(&new_expr->as.self_call.args);
  } break;
  }
}

static void clone_block(IrBlock *block) {
  IrBlock new_block = {0};
  new_block.cap = block->cap;
  new_block.len = block->len;
  new_block.items = malloc(new_block.cap * sizeof(IrExpr *));
  memcpy(new_block.items, block->items, new_block.len * sizeof(IrExpr *));

  for (u32 i = 0; i < new_block.len; ++i)
    clone_expr(new_block.items + i);

  *block = new_block;
}

static Macro *get_macro(Macros *macros, Str name, u32 args_len) {
  for (u32 i = 0; i < macros->len; ++i) {
    Macro *macro = macros->items + i;

    if (str_eq(macro->name, name) &&
        (macro->arg_names.len == args_len ||
         (macro->arg_names.len < args_len &&
          macro->has_unpack)))
      return macro;
  }

  return NULL;
}

static u32 get_macro_arg_index(Str name, IrArgs *arg_names) {
  for (u32 i = 0; i < arg_names->len; ++i)
    if (str_eq(arg_names->items[i], name))
      return i;

  return (u32) -1;
}

static bool try_inline_macro_arg(IrExpr **expr, IrArgs *arg_names,
                                 IrBlock *args, IrBlock *dest,
                                 bool unpack);

static void append_macro_arg(u32 index, IrArgs *arg_names,
                             IrBlock *args, IrBlock *dest,
                             bool unpack) {
  IrExpr *arg = args->items[index];

  if (unpack && index + 1 == args->len && arg->kind == IrExprKindList) {
    for (u32 i = 0; i < arg->as.list.content.len; ++i) {
      if (!try_inline_macro_arg(arg->as.list.content.items + i,
                                arg_names, args, dest, unpack)) {
        IrExpr *new_arg = arg->as.list.content.items[i];
        clone_expr(&new_arg);
        DA_APPEND(*dest, new_arg);
      }
    }
  } else {
    if (!try_inline_macro_arg(&arg, arg_names, args, dest, unpack)) {
      clone_expr(&arg);
      DA_APPEND(*dest, arg);
    }
  }
}

static bool try_inline_macro_arg(IrExpr **expr, IrArgs *arg_names,
                                 IrBlock *args, IrBlock *dest,
                                 bool unpack) {
  if (!arg_names || !args)
    return false;

  if ((*expr)->kind == IrExprKindVarDef) {
    u32 arg_index = get_macro_arg_index((*expr)->as.var_def.name, arg_names);
    if (arg_index != (u32) -1 && args->items[arg_index]->kind == IrExprKindIdent)
      (*expr)->as.var_def.name = args->items[arg_index]->as.ident.ident;

    // It is ok!
    return false;
  }

  if ((*expr)->kind == IrExprKindIdent) {
    u32 arg_index = get_macro_arg_index((*expr)->as.ident.ident, arg_names);
    if (arg_index != (u32) -1) {
      if (dest) {
        append_macro_arg(arg_index, arg_names, args, dest, unpack);
      } else {
        IrExpr *arg = args->items[arg_index];
        clone_expr(&arg);
        *expr = arg;
      }

      return true;
    }
  }

  return false;
}

void expand_macros_block(IrBlock *block, Macros *macros,
                         IrArgs *arg_names, IrBlock *args,
                         bool unpack) {
  IrBlock new_block = {0};

  for (u32 i = 0; i < block->len; ++i) {
    IrExpr *new_expr = block->items[i];
    clone_expr(&new_expr);

    if (!try_inline_macro_arg(&new_expr, arg_names, args, &new_block, unpack))
      DA_APPEND(new_block, new_expr);
  }

  for (u32 i = 0; i < new_block.len; ++i)
    expand_macros(new_block.items[i], macros, arg_names, args, unpack);

  *block = new_block;
}

#define INLINE_THEN_EXPAND(expr)                                  \
  do {                                                            \
    try_inline_macro_arg(&(expr), arg_names, args, NULL, unpack); \
    expand_macros(expr, macros, arg_names, args, unpack);         \
  } while (0)

void expand_macros(IrExpr *expr, Macros *macros,
                   IrArgs *arg_names, IrBlock *args,
                   bool unpack) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    expand_macros_block(&expr->as.block, macros, arg_names, args, unpack);
  } break;

  case IrExprKindFuncCall: {
    INLINE_THEN_EXPAND(expr->as.func_call.func);
    expand_macros_block(&expr->as.func_call.args, macros, arg_names, args, unpack);

    if (expr->as.func_call.func->kind == IrExprKindIdent) {
      Str name = expr->as.func_call.func->as.ident.ident;
      Macro *macro = get_macro(macros, name, expr->as.func_call.args.len);

      if (macro) {
        IrBlock new_args = {0};

        if (macro->has_unpack) {
          for (u32 i = 0; i < macro->arg_names.len - 1; ++i)
            DA_APPEND(new_args, expr->as.func_call.args.items[i]);

          IrBlock variadic_args = {0};

          for (u32 i = macro->arg_names.len - 1; i < expr->as.func_call.args.len; ++i)
            DA_APPEND(variadic_args, expr->as.func_call.args.items[i]);

          IrExpr *new_arg = aalloc(sizeof(IrExpr));
          new_arg->kind = IrExprKindList;
          new_arg->as.list.content = variadic_args;
          DA_APPEND(new_args, new_arg);
        } else {
          for (u32 i = 0; i < expr->as.func_call.args.len; ++i)
            DA_APPEND(new_args, expr->as.func_call.args.items[i]);
        }

        expr->kind = IrExprKindBlock;
        expr->as.block = macro->body;
        clone_block(&expr->as.block);

        expand_macros_block(&expr->as.block, macros,
                            &macro->arg_names, &new_args,
                            macro->has_unpack);

        free(new_args.items);
      }
    }
  } break;

  case IrExprKindVarDef: {
    INLINE_THEN_EXPAND(expr->as.var_def.expr);
  } break;

  case IrExprKindIf: {
    INLINE_THEN_EXPAND(expr->as._if.cond);
    expand_macros_block(&expr->as._if.if_body, macros, arg_names, args, unpack);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      INLINE_THEN_EXPAND(expr->as._if.elifs.items[i].cond);
      expand_macros_block(&expr->as._if.elifs.items[i].body, macros,
                          arg_names, args, unpack);
    }

    if (expr->as._if.has_else)
      expand_macros_block(&expr->as._if.else_body, macros, arg_names, args, unpack);
  } break;

  case IrExprKindWhile: {
    INLINE_THEN_EXPAND(expr->as._while.cond);
    expand_macros_block(&expr->as._while.body, macros, arg_names, args, unpack);
  } break;

  case IrExprKindSet: {
    INLINE_THEN_EXPAND(expr->as.set.src);
  } break;

  case IrExprKindField: {
    INLINE_THEN_EXPAND(expr->as.field.record);
    INLINE_THEN_EXPAND(expr->as.field.expr);
  } break;

  case IrExprKindList: {
    expand_macros_block(&expr->as.list.content, macros, arg_names, args, unpack);
  } break;

  case IrExprKindIdent:  break;
  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    expand_macros_block(&expr->as.lambda.body, macros, arg_names, args, unpack);
  } break;

  case IrExprKindRecord: {
    for (u32 i = 0; i < expr->as.record.len; ++i)
      INLINE_THEN_EXPAND(expr->as.record.items[i].expr);
  } break;

  case IrExprKindRet: {
    INLINE_THEN_EXPAND(expr->as.ret.expr);
  } break;

  case IrExprKindSelfCall: {
    expand_macros_block(&expr->as.self_call.args, macros, arg_names, args, unpack);
  } break;
  }
}
