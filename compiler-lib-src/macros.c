#include "macros.h"
#include "shl/shl-arena.h"

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

  case IrExprKindGet: {
    clone_expr(&new_expr->as.get.key);
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

  case IrExprKindDict: {
    for (u32 i = 0; i < new_expr->as.dict.len; ++i)
      clone_expr(&new_expr->as.dict.items[i].expr);
  } break;

  case IrExprKindRet: {
    if (new_expr->as.ret.has_expr)
      clone_expr(&new_expr->as.ret.expr);
  } break;

  case IrExprKindSelfCall: {
    clone_block(&new_expr->as.self_call.args);
  } break;

  case IrExprKindSetIn: {
    for (u32 i = 0; i < new_expr->as.set_in.fields.len; ++i) {
      clone_expr(&new_expr->as.set_in.fields.items[i].key);
      clone_expr(&new_expr->as.set_in.fields.items[i].expr);
    }
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

static void rename_args_block(IrBlock *block, IrArgs *prev_arg_names, IrArgs *new_arg_names);

static void rename_args_expr(IrExpr *expr, IrArgs *prev_arg_names, IrArgs *new_arg_names) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    rename_args_block(&expr->as.block, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindFuncCall: {
    rename_args_expr(expr->as.func_call.func, prev_arg_names, new_arg_names);
    rename_args_block(&expr->as.func_call.args, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindVarDef: {
    rename_args_expr(expr->as.var_def.expr, prev_arg_names, new_arg_names);

    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.var_def.name, prev_arg_names->items[i])) {
        expr->as.var_def.name = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case IrExprKindIf: {
    rename_args_expr(expr->as._if.cond, prev_arg_names, new_arg_names);
    rename_args_block(&expr->as._if.if_body, prev_arg_names, new_arg_names);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      rename_args_expr(expr->as._if.elifs.items[i].cond, prev_arg_names, new_arg_names);
      rename_args_block(&expr->as._if.elifs.items[i].body, prev_arg_names, new_arg_names);
    }

    if (expr->as._if.has_else)
      rename_args_block(&expr->as._if.else_body, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindWhile: {
    rename_args_expr(expr->as._while.cond, prev_arg_names, new_arg_names);
    rename_args_block(&expr->as._while.body, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindSet: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.set.dest, prev_arg_names->items[i])) {
        expr->as.set.dest = new_arg_names->items[i];

        break;
      }
    }

    rename_args_expr(expr->as.set.src, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindGet: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.get.src, prev_arg_names->items[i])) {
        expr->as.get.src = new_arg_names->items[i];

        break;
      }
    }

    rename_args_expr(expr->as.get.key, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindList: {
    rename_args_block(&expr->as.list.content, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.ident.ident, prev_arg_names->items[i])) {
        expr->as.ident.ident = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    rename_args_block(&expr->as.lambda.body, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i)
      rename_args_expr(expr->as.dict.items[i].expr, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      rename_args_expr(expr->as.ret.expr, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindSelfCall: {
    rename_args_block(&expr->as.self_call.args, prev_arg_names, new_arg_names);
  } break;

  case IrExprKindSetIn: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.set_in.dict, prev_arg_names->items[i])) {
        expr->as.set_in.dict = new_arg_names->items[i];

        break;
      }
    }

    for (u32 i = 0; i < expr->as.set_in.fields.len; ++i) {
      rename_args_expr(expr->as.set_in.fields.items[i].key, prev_arg_names, new_arg_names);
      rename_args_expr(expr->as.set_in.fields.items[i].expr, prev_arg_names, new_arg_names);
    }
  } break;
  }
}

static void rename_args_block(IrBlock *block, IrArgs *prev_arg_names, IrArgs *new_arg_names) {
  for (u32 i = 0; i < block->len; ++i)
    rename_args_expr(block->items[i], prev_arg_names, new_arg_names);
}

static u32 get_macro_arg_index(Str name, IrArgs *arg_names) {
  for (u32 i = 0; i < arg_names->len; ++i)
    if (str_eq(arg_names->items[i], name))
      return i;

  return (u32) -1;
}

static void try_replace_macro_arg_ident(Str *ident, IrArgs *arg_names, IrBlock *args) {
  u32 arg_index = get_macro_arg_index(*ident, arg_names);
  if (arg_index != (u32) -1 && args->items[arg_index]->kind == IrExprKindIdent)
    *ident = args->items[arg_index]->as.ident.ident;
}

static bool try_inline_macro_arg(IrExpr **expr, IrArgs *arg_names,
                                 IrBlock *args, IrBlock *dest);

static void append_macro_arg(u32 index, IrArgs *arg_names,
                             IrBlock *args, IrBlock *dest) {
  IrExpr *arg = args->items[index];

  if (args->len > arg_names->len) {
    for (u32 i = arg_names->len - 1; i < args->len; ++i) {
      IrExpr *new_arg = args->items[i];

      if (!try_inline_macro_arg(&new_arg, arg_names, args, dest)) {
        clone_expr(&new_arg);
        DA_APPEND(*dest, new_arg);
      }
    }
  } else {
    clone_expr(&arg);
    DA_APPEND(*dest, arg);
  }
}

static bool try_inline_macro_arg(IrExpr **expr, IrArgs *arg_names,
                                 IrBlock *args, IrBlock *dest) {
  if (!arg_names || !args)
    return false;

  if ((*expr)->kind == IrExprKindVarDef) {
    try_replace_macro_arg_ident(&(*expr)->as.var_def.name, arg_names, args);

    return false;
  } else if ((*expr)->kind == IrExprKindSet) {
    try_replace_macro_arg_ident(&(*expr)->as.set.dest, arg_names, args);

    return false;
  } else if ((*expr)->kind == IrExprKindGet) {
    try_replace_macro_arg_ident(&(*expr)->as.get.src, arg_names, args);

    return false;
  } else if ((*expr)->kind == IrExprKindSetIn) {
    try_replace_macro_arg_ident(&(*expr)->as.set_in.dict, arg_names, args);

    return false;
  }

  if ((*expr)->kind == IrExprKindIdent) {
    u32 arg_index = get_macro_arg_index((*expr)->as.ident.ident, arg_names);
    if (arg_index != (u32) -1) {
      if (dest) {
        append_macro_arg(arg_index, arg_names, args, dest);
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
                         IrArgs *arg_names, IrBlock *args) {
  IrBlock new_block = {0};

  for (u32 i = 0; i < block->len; ++i) {
    IrExpr *new_expr = block->items[i];

    if (!try_inline_macro_arg(&new_expr, arg_names, args, &new_block)) {
      clone_expr(&new_expr);
      DA_APPEND(new_block, new_expr);
    }
  }

  for (u32 i = 0; i < new_block.len; ++i)
    expand_macros(new_block.items[i], macros, arg_names, args);

  *block = new_block;
}

#define INLINE_THEN_EXPAND(expr)                          \
  do {                                                    \
    try_inline_macro_arg(&(expr), arg_names, args, NULL); \
    expand_macros(expr, macros, arg_names, args);         \
  } while (0)

void expand_macros(IrExpr *expr, Macros *macros,
                   IrArgs *arg_names, IrBlock *args) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    expand_macros_block(&expr->as.block, macros, arg_names, args);
  } break;

  case IrExprKindFuncCall: {
    INLINE_THEN_EXPAND(expr->as.func_call.func);
    expand_macros_block(&expr->as.func_call.args, macros, arg_names, args);

    if (expr->as.func_call.func->kind == IrExprKindIdent) {
      Str name = expr->as.func_call.func->as.ident.ident;
      Macro *macro = get_macro(macros, name, expr->as.func_call.args.len);

      if (macro) {
        IrBlock new_args = expr->as.func_call.args;
        clone_block(&new_args);

        IrArgs new_arg_names = {0};

        for (u32 i = 0; i < macro->arg_names.len; ++i) {
          StringBuilder sb = {0};
          sb_push_u64(&sb, (u64) &macro);
          sb_push_char(&sb, '@');
          sb_push_str(&sb, macro->arg_names.items[i]);

          Str new_arg_name = sb_to_str(sb);
          DA_APPEND(new_arg_names, new_arg_name);
        }

        expr->kind = IrExprKindBlock;
        expr->as.block = macro->body;
        clone_block(&expr->as.block);

        rename_args_block(&expr->as.block, &macro->arg_names, &new_arg_names);
        expand_macros_block(&expr->as.block, macros,
                            &new_arg_names, &new_args);
      }
    }
  } break;

  case IrExprKindVarDef: {
    INLINE_THEN_EXPAND(expr->as.var_def.expr);
  } break;

  case IrExprKindIf: {
    INLINE_THEN_EXPAND(expr->as._if.cond);
    expand_macros_block(&expr->as._if.if_body, macros, arg_names, args);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      INLINE_THEN_EXPAND(expr->as._if.elifs.items[i].cond);
      expand_macros_block(&expr->as._if.elifs.items[i].body, macros, arg_names, args);
    }

    if (expr->as._if.has_else)
      expand_macros_block(&expr->as._if.else_body, macros, arg_names, args);
  } break;

  case IrExprKindWhile: {
    INLINE_THEN_EXPAND(expr->as._while.cond);
    expand_macros_block(&expr->as._while.body, macros, arg_names, args);
  } break;

  case IrExprKindSet: {
    INLINE_THEN_EXPAND(expr->as.set.src);
  } break;

  case IrExprKindGet: {
    INLINE_THEN_EXPAND(expr->as.get.key);
  } break;

  case IrExprKindList: {
    expand_macros_block(&expr->as.list.content, macros, arg_names, args);
  } break;

  case IrExprKindIdent:  break;
  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    expand_macros_block(&expr->as.lambda.body, macros, arg_names, args);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i)
      INLINE_THEN_EXPAND(expr->as.dict.items[i].expr);
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      INLINE_THEN_EXPAND(expr->as.ret.expr);
  } break;

  case IrExprKindSelfCall: {
    expand_macros_block(&expr->as.self_call.args, macros, arg_names, args);
  } break;

  case IrExprKindSetIn: {
    for (u32 i = 0; i < expr->as.set_in.fields.len; ++i) {
      INLINE_THEN_EXPAND(expr->as.set_in.fields.items[i].key);
      INLINE_THEN_EXPAND(expr->as.set_in.fields.items[i].expr);
    }
  } break;
  }
}
