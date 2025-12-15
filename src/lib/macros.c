#include "aether/macros.h"

#define INLINE_THEN_EXPAND(expr)                                            \
  do {                                                                      \
    try_inline_macro_arg(&(expr), arg_names, args, NULL, unpack, arena);    \
    expand_macros(expr, macros, arg_names, args, unpack, arena, file_path); \
  } while (0)

static u32 get_macro_arg_index(Str name, IrArgs *arg_names) {
  for (u32 i = 0; i < arg_names->len; ++i)
    if (str_eq(arg_names->items[i], name))
      return i;

  return (u32) -1;
}

static bool needs_cloning(IrExpr *expr, IrArgs *arg_names) {
  switch (expr->kind) {
    case IrExprKindIdent: {
      return get_macro_arg_index(expr->as.ident, arg_names) != (u32) -1;
    }

    case IrExprKindInt:
    case IrExprKindFloat:
    case IrExprKindBool:
    case IrExprKindString: {
      return false;
    }

    case IrExprKindRet: {
      return expr->as.ret.has_expr;
    }

    case IrExprKindSelf: {
      return false;
    }

    default: {
      return true;
    }
  }
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

static void clone_block(IrBlock *block, IrArgs *arg_names, Arena *arena);

static void clone_expr(IrExpr **expr, IrArgs *arg_names, Arena *arena) {
  if (!arg_names || !needs_cloning(*expr, arg_names))
    return;

  IrExpr *new_expr = arena_alloc(arena, sizeof(IrExpr));
  *new_expr = **expr;
  *expr = new_expr;

  switch (new_expr->kind) {
  case IrExprKindBlock: {
    clone_block(&new_expr->as.block.block, arg_names, arena);
  } break;

  case IrExprKindFuncCall: {
    clone_expr(&new_expr->as.func_call.func, arg_names, arena);
    clone_block(&new_expr->as.func_call.args, arg_names, arena);
  } break;

  case IrExprKindVarDef: {
    clone_expr(&new_expr->as.var_def.expr, arg_names, arena);
  } break;

  case IrExprKindIf: {
    clone_expr(&new_expr->as._if.cond, arg_names, arena);
    clone_block(&new_expr->as._if.if_body, arg_names, arena);

    for (u32 i = 0; i < new_expr->as._if.elifs.len; ++i) {
      clone_expr(&new_expr->as._if.elifs.items[i].cond, arg_names, arena);
      clone_block(&new_expr->as._if.elifs.items[i].body, arg_names, arena);
    }

    if (new_expr->as._if.has_else)
      clone_block(&new_expr->as._if.else_body, arg_names, arena);
  } break;

  case IrExprKindWhile: {
    clone_expr(&new_expr->as._while.cond, arg_names, arena);
    clone_block(&new_expr->as._while.body, arg_names, arena);
  } break;

  case IrExprKindSet: {
    clone_expr(&new_expr->as.set.src, arg_names, arena);
  } break;

  case IrExprKindGetAt: {
    clone_expr(&new_expr->as.get_at.src, arg_names, arena);
    clone_expr(&new_expr->as.get_at.key, arg_names, arena);
  } break;

  case IrExprKindSetAt: {
    clone_expr(&new_expr->as.set_at.key, arg_names, arena);
    clone_expr(&new_expr->as.set_at.value, arg_names, arena);
  } break;

  case IrExprKindList: {
    clone_block(&new_expr->as.list, arg_names, arena);
  } break;

  case IrExprKindIdent:  break;
  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    Str *new_items = arena_alloc(arena, new_expr->as.lambda.args.len * sizeof(Str));
    memcpy(new_items, new_expr->as.lambda.args.items, new_expr->as.lambda.args.len * sizeof(Str));
    new_expr->as.lambda.args.items = new_items;

    clone_block(&new_expr->as.lambda.body, arg_names, arena);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < new_expr->as.dict.len; ++i) {
      clone_expr(&new_expr->as.dict.items[i].key, arg_names, arena);
      clone_expr(&new_expr->as.dict.items[i].expr, arg_names, arena);
    }
  } break;

  case IrExprKindRet: {
    if (new_expr->as.ret.has_expr)
      clone_expr(&new_expr->as.ret.expr, arg_names, arena);
  } break;

  case IrExprKindMatch: {
    clone_expr(&new_expr->as.match.src, arg_names, arena);

    for (u32 i = 0; i < new_expr->as.match.cases.len; ++i) {
      clone_expr(&new_expr->as.match.cases.items[i].pattern, arg_names, arena);
      clone_expr(&new_expr->as.match.cases.items[i].expr, arg_names, arena);
    }
  } break;

  case IrExprKindSelf: break;
  }
}

static void clone_block(IrBlock *block, IrArgs *arg_names, Arena *arena) {
  IrBlock new_block = {0};
  new_block.len = block->len;
  new_block.items = arena_alloc(arena, new_block.len * sizeof(IrExpr *));
  memcpy(new_block.items, block->items, new_block.len * sizeof(IrExpr *));

  for (u32 i = 0; i < new_block.len; ++i)
    clone_expr(new_block.items + i, arg_names, arena);

  *block = new_block;
}

static void rename_args_block(IrBlock *block, IrArgs *prev_arg_names,
                              IrArgs *new_arg_names, Arena *arena);

static void rename_args_expr(IrExpr *expr, IrArgs *prev_arg_names,
                             IrArgs *new_arg_names, Arena *arena) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    rename_args_block(&expr->as.block.block, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindFuncCall: {
    rename_args_expr(expr->as.func_call.func, prev_arg_names, new_arg_names, arena);
    rename_args_block(&expr->as.func_call.args, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindVarDef: {
    rename_args_expr(expr->as.var_def.expr, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.var_def.name, prev_arg_names->items[i])) {
        expr->as.var_def.name = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case IrExprKindIf: {
    rename_args_expr(expr->as._if.cond, prev_arg_names, new_arg_names, arena);
    rename_args_block(&expr->as._if.if_body, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      rename_args_expr(expr->as._if.elifs.items[i].cond, prev_arg_names, new_arg_names, arena);
      rename_args_block(&expr->as._if.elifs.items[i].body, prev_arg_names, new_arg_names, arena);
    }

    if (expr->as._if.has_else)
      rename_args_block(&expr->as._if.else_body, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindWhile: {
    rename_args_expr(expr->as._while.cond, prev_arg_names, new_arg_names, arena);
    rename_args_block(&expr->as._while.body, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindSet: {
    rename_args_expr(expr->as.set.src, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.set.dest, prev_arg_names->items[i])) {
        expr->as.set.dest = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case IrExprKindGetAt: {
    rename_args_expr(expr->as.get_at.src, prev_arg_names, new_arg_names, arena);
    rename_args_expr(expr->as.get_at.key, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindSetAt: {
    rename_args_expr(expr->as.set_at.key, prev_arg_names, new_arg_names, arena);
    rename_args_expr(expr->as.set_at.value, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.set_at.dest, prev_arg_names->items[i])) {
        expr->as.set_at.dest = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case IrExprKindList: {
    rename_args_block(&expr->as.list, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindIdent: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (str_eq(expr->as.ident, prev_arg_names->items[i])) {
        expr->as.ident = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    for (u32 i = 0; i < expr->as.lambda.args.len; ++i) {
      for (u32 j = 0; j < prev_arg_names->len; ++j) {
        if (str_eq(expr->as.lambda.args.items[i], prev_arg_names->items[j])) {
          expr->as.lambda.args.items[i] = new_arg_names->items[j];

          break;
        }
      }
    }

    rename_args_block(&expr->as.lambda.body, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      rename_args_expr(expr->as.dict.items[i].key, prev_arg_names, new_arg_names, arena);
      rename_args_expr(expr->as.dict.items[i].expr, prev_arg_names, new_arg_names, arena);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      rename_args_expr(expr->as.ret.expr, prev_arg_names, new_arg_names, arena);
  } break;

  case IrExprKindMatch: {
    rename_args_expr(expr->as.match.src, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      rename_args_expr(expr->as.match.cases.items[i].pattern, prev_arg_names, new_arg_names, arena);
      rename_args_expr(expr->as.match.cases.items[i].expr, prev_arg_names, new_arg_names, arena);
    }
  } break;

  case IrExprKindSelf: break;
  }
}

static void rename_args_block(IrBlock *block, IrArgs *prev_arg_names,
                              IrArgs *new_arg_names, Arena *arena) {
  for (u32 i = 0; i < block->len; ++i)
    rename_args_expr(block->items[i], prev_arg_names, new_arg_names, arena);
}

static void try_replace_macro_arg_ident(Str *ident, IrArgs *arg_names, IrBlock *args) {
  u32 arg_index = get_macro_arg_index(*ident, arg_names);
  if (arg_index != (u32) -1 && args->items[arg_index]->kind == IrExprKindIdent)
    *ident = args->items[arg_index]->as.ident;
}

static bool try_inline_macro_arg(IrExpr **expr, IrArgs *arg_names,
                                 IrBlock *args, Block *dest,
                                 bool unpack, Arena *arena);

static void append_macro_arg(u32 index, IrArgs *arg_names,
                             IrBlock *args, Block *dest,
                             bool unpack, Arena *arena) {
  IrExpr *arg = args->items[index];

  if (unpack && index + 1 == arg_names->len) {
    IrBlock *variadic_args = &args->items[args->len - 1]->as.list;
    for (u32 i = 0; i < variadic_args->len; ++i) {
      IrExpr *new_arg = variadic_args->items[i];

      if (!try_inline_macro_arg(&new_arg, arg_names, args, dest, unpack, arena)) {
        clone_expr(&new_arg, arg_names, arena);
        block_append(dest, new_arg, arena);
      }
    }
  } else {
    clone_expr(&arg, arg_names, arena);
    block_append(dest, arg, arena);
  }
}

static bool try_inline_macro_arg(IrExpr **expr, IrArgs *arg_names,
                                 IrBlock *args, Block *dest,
                                 bool unpack, Arena *arena) {
  if (!arg_names || !args)
    return false;

  if ((*expr)->kind == IrExprKindVarDef) {
    try_replace_macro_arg_ident(&(*expr)->as.var_def.name, arg_names, args);

    return false;
  }

  if ((*expr)->kind == IrExprKindSet) {
    try_replace_macro_arg_ident(&(*expr)->as.set.dest, arg_names, args);

    return false;
  }

  if ((*expr)->kind == IrExprKindSetAt) {
    try_replace_macro_arg_ident(&(*expr)->as.set_at.dest, arg_names, args);

    return false;
  }

  if ((*expr)->kind == IrExprKindLambda) {
    for (u32 i = 0; i < (*expr)->as.lambda.args.len; ++i)
      try_replace_macro_arg_ident((*expr)->as.lambda.args.items + i, arg_names, args);

    return false;
  }

  if ((*expr)->kind == IrExprKindIdent) {
    u32 arg_index = get_macro_arg_index((*expr)->as.ident, arg_names);
    if (arg_index != (u32) -1) {
      if (dest) {
        append_macro_arg(arg_index, arg_names, args, dest, unpack, arena);
      } else {
        IrExpr *arg = args->items[arg_index];
        clone_expr(&arg, arg_names, arena);
        *expr = arg;
      }

      return true;
    }
  }

  return false;
}

void expand_macros_block(IrBlock *block, Macros *macros,
                         IrArgs *arg_names, IrBlock *args,
                         bool unpack, Arena *arena,
                         Str file_path) {
  Block new_block = {0};

  for (u32 i = 0; i < block->len; ++i) {
    IrExpr *new_expr = block->items[i];

    if (!try_inline_macro_arg(&new_expr, arg_names, args, &new_block, unpack, arena)) {
      clone_expr(&new_expr, arg_names, arena);
      block_append(&new_block, new_expr, arena);
    }
  }

  for (u32 i = 0; i < new_block.len; ++i)
    expand_macros(new_block.items[i], macros, arg_names, args, unpack, arena, file_path);

  block->items = new_block.items;
  block->len = new_block.len;
}

void expand_macros(IrExpr *expr, Macros *macros,
                   IrArgs *arg_names, IrBlock *args,
                   bool unpack, Arena *arena,
                   Str file_path) {
  switch (expr->kind) {
  case IrExprKindBlock: {
    expand_macros_block(&expr->as.block.block, macros, arg_names,
                        args, unpack, arena, file_path);
  } break;

  case IrExprKindFuncCall: {
    INLINE_THEN_EXPAND(expr->as.func_call.func);
    expand_macros_block(&expr->as.func_call.args, macros, arg_names,
                        args, unpack, arena, file_path);

    if (expr->as.func_call.func->kind == IrExprKindIdent) {
      Str name = expr->as.func_call.func->as.ident;
      Macro *macro = get_macro(macros, name, expr->as.func_call.args.len);

      if (macro) {
        Block new_args = {
          expr->as.func_call.args.items,
          macro->arg_names.len,
          macro->arg_names.len,
        };

        if (macro->has_unpack) {
          --new_args.len;

          Block variadic_block;
          variadic_block.len = expr->as.func_call.args.len - new_args.len;
          variadic_block.cap = variadic_block.len;
          variadic_block.items = arena_alloc(arena, variadic_block.cap * sizeof(IrExpr *));

          for (u32 i = 0; i < variadic_block.len; ++i)
            variadic_block.items[i] = expr->as.func_call.args.items[new_args.len + i];

          IrExpr *variadic_args = arena_alloc(arena, sizeof(IrExpr));
          variadic_args->kind = IrExprKindList;
          variadic_args->as.list.items = variadic_block.items;
          variadic_args->as.list.len = variadic_block.len;
          DA_APPEND(new_args, variadic_args);
        }

        Args new_arg_names = {0};

        for (u32 i = 0; i < macro->arg_names.len; ++i) {
          StringBuilder sb = {0};
          sb_push_str(&sb, macro->name);
          sb_push_char(&sb, '@');
          sb_push_str(&sb, macro->arg_names.items[i]);

          Str new_arg_name = copy_str(sb_to_str(sb), arena);
          DA_APPEND(new_arg_names, new_arg_name);

          free(sb.buffer);
        };

        expr->kind = IrExprKindBlock;
        expr->as.block.block = macro->body;
        expr->as.block.file_path = file_path;

        IrExpr **new_items = arena_alloc(arena, expr->as.block.block.len * sizeof(IrExpr *));
        memcpy(new_items, expr->as.block.block.items, expr->as.block.block.len * sizeof(IrExpr *));
        expr->as.block.block.items = new_items;

        IrArgs ir_new_arg_names = {
          new_arg_names.items,
          new_arg_names.len,
        };

        IrBlock ir_new_args = {
          new_args.items,
          new_args.len,
        };

        rename_args_block(&expr->as.block.block, &macro->arg_names,
                          &ir_new_arg_names, arena);
        expand_macros_block(&expr->as.block.block, macros,
                            &ir_new_arg_names, &ir_new_args,
                            macro->has_unpack, arena, file_path);

        free(new_arg_names.items);

        break;
      }
    }
  } break;

  case IrExprKindVarDef: {
    INLINE_THEN_EXPAND(expr->as.var_def.expr);
  } break;

  case IrExprKindIf: {
    INLINE_THEN_EXPAND(expr->as._if.cond);
    expand_macros_block(&expr->as._if.if_body, macros, arg_names,
                        args, unpack, arena, file_path);

    for (u32 i = 0; i < expr->as._if.elifs.len; ++i) {
      INLINE_THEN_EXPAND(expr->as._if.elifs.items[i].cond);
      expand_macros_block(&expr->as._if.elifs.items[i].body, macros,
                          arg_names, args, unpack, arena, file_path);
    }

    if (expr->as._if.has_else)
      expand_macros_block(&expr->as._if.else_body, macros, arg_names,
                          args, unpack, arena, file_path);
  } break;

  case IrExprKindWhile: {
    INLINE_THEN_EXPAND(expr->as._while.cond);
    expand_macros_block(&expr->as._while.body, macros, arg_names,
                        args, unpack, arena, file_path);
  } break;

  case IrExprKindSet: {
    INLINE_THEN_EXPAND(expr->as.set.src);
  } break;

  case IrExprKindGetAt: {
    INLINE_THEN_EXPAND(expr->as.get_at.src);
    INLINE_THEN_EXPAND(expr->as.get_at.key);
  } break;

  case IrExprKindSetAt: {
    INLINE_THEN_EXPAND(expr->as.set_at.key);
    INLINE_THEN_EXPAND(expr->as.set_at.value);
  } break;

  case IrExprKindList: {
    expand_macros_block(&expr->as.list, macros, arg_names, args, unpack, arena, file_path);
  } break;

  case IrExprKindIdent:  break;
  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    expand_macros_block(&expr->as.lambda.body, macros, arg_names, args, unpack, arena, file_path);
  } break;

  case IrExprKindDict: {
    for (u32 i = 0; i < expr->as.dict.len; ++i) {
      INLINE_THEN_EXPAND(expr->as.dict.items[i].key);
      INLINE_THEN_EXPAND(expr->as.dict.items[i].expr);
    }
  } break;

  case IrExprKindRet: {
    if (expr->as.ret.has_expr)
      INLINE_THEN_EXPAND(expr->as.ret.expr);
  } break;

  case IrExprKindMatch: {
    INLINE_THEN_EXPAND(expr->as.match.src);

    for (u32 i = 0; i < expr->as.match.cases.len; ++i) {
      INLINE_THEN_EXPAND(expr->as.match.cases.items[i].pattern);
      INLINE_THEN_EXPAND(expr->as.match.cases.items[i].expr);
    }
  } break;

  case IrExprKindSelf: break;
  }
}
