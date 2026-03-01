#include "aether/macros.h"

#define INLINE_THEN_EXPAND(expr)                                                        \
  do {                                                                                  \
    bool inlined = try_inline_macro_arg(&(expr), arg_names, args, NULL, unpack, arena); \
    if (inlined) {                                                                      \
      row = 0;                                                                          \
      col = 0;                                                                          \
    }                                                                                   \
    expand_macros(expr, macros, arg_names, args, unpack,                                \
                  arena, file_path, row, col, is_inlined | inlined);                    \
  } while (0)

#define INLINE_THEN_EXPAND_BLOCK(block)                 \
  do {                                                  \
    expand_macros_block(&block, macros, arg_names,      \
                        args, unpack, arena, file_path, \
                        row, col, is_inlined);          \
  } while (0)

void block_append(DaExprs *block, Expr *expr, Arena *arena) {
  if (!block->items) {
    block->cap = 1;
    block->items = arena_alloc(arena, sizeof(Expr *));
  } else if (block->cap <= block->len) {
    block->cap *= 2;

    Expr **new_items = arena_alloc(arena, block->cap * sizeof(Expr *));
    memcpy(new_items, block->items, block->len * sizeof(Expr *));
    block->items = new_items;
  }

  block->items[block->len++] = expr;
}

static u32 get_macro_arg_index(u16 name_id, Args *arg_names) {
  for (u32 i = 0; i < arg_names->len; ++i)
    if (arg_names->items[i] == name_id)
      return i;

  return (u32) -1;
}

static bool needs_cloning(Expr *expr, Args *arg_names) {
  switch (expr->kind) {
    case ExprKindIdent: {
      return get_macro_arg_index(expr->as.ident.name_id, arg_names) != (u32) -1;
    }

    case ExprKindRet: {
      return expr->as.ret.value != NULL;
    }

    case ExprKindSelf: {
      return false;
    }

    default: {
      return true;
    }
  }
}

static Macro *get_macro(Macros *macros, u16 name_id, u32 args_len) {
  for (u32 i = 0; i < macros->len; ++i) {
    Macro *macro = macros->items + i;

    if (macro->name_id == name_id &&
        (macro->arg_names.len == args_len ||
         (macro->has_unpack &&
          macro->arg_names.len <= args_len + 1)))
      return macro;
  }

  return NULL;
}

static void clone_block(Exprs *block, Args *arg_names, Arena *arena);

static void clone_expr(Expr **expr, Args *arg_names, Arena *arena) {
  if (!arg_names || !needs_cloning(*expr, arg_names))
    return;

  Expr *new_expr = arena_alloc(arena, sizeof(Expr));
  *new_expr = **expr;
  *expr = new_expr;

  switch (new_expr->kind) {
  case ExprKindString: break;
  case ExprKindInt: break;
  case ExprKindFloat: break;
  case ExprKindBytes: break;

  case ExprKindBlock: {
    clone_block(&new_expr->as.block, arg_names, arena);
  } break;

  case ExprKindIdent: break;

  case ExprKindFunc: {
    u16 *new_items = arena_alloc(arena, new_expr->as.func.args.len * sizeof(u16));
    memcpy(new_items, new_expr->as.func.args.items, new_expr->as.func.args.len * sizeof(u16));
    new_expr->as.func.args.items = new_items;

    clone_block(&new_expr->as.func.body, arg_names, arena);
  } break;

  case ExprKindList: {
    clone_block(&new_expr->as.list.content, arg_names, arena);
  } break;

  case ExprKindDict: {
    clone_block(&new_expr->as.dict.content, arg_names, arena);
  } break;

  case ExprKindGet: {
    clone_block(&new_expr->as.get.chain, arg_names, arena);
  } break;

  case ExprKindSet: {
    clone_block(&new_expr->as.set.chain, arg_names, arena);
    clone_expr(&new_expr->as.set.new, arg_names, arena);
  } break;

  case ExprKindFuncCall: {
    clone_expr(&new_expr->as.func_call.func, arg_names, arena);
    clone_block(&new_expr->as.func_call.args, arg_names, arena);
  } break;

  case ExprKindLet: {
    clone_expr(&new_expr->as.let.value, arg_names, arena);
  } break;

  case ExprKindRet: {
    if (new_expr->as.ret.value)
      clone_expr(&new_expr->as.ret.value, arg_names, arena);
  } break;

  case ExprKindIf: {
    clone_expr(&new_expr->as._if.cond, arg_names, arena);
    clone_block(&new_expr->as._if.if_body, arg_names, arena);
    clone_block(&new_expr->as._if.else_body, arg_names, arena);
  } break;

  case ExprKindMatch: {
    clone_expr(&new_expr->as.match.value, arg_names, arena);

    for (u32 i = 0; i < new_expr->as.match.branches.len; ++i) {
      if (new_expr->as.match.branches.items[i].value)
        clone_expr(&new_expr->as.match.branches.items[i].value, arg_names, arena);
      clone_expr(&new_expr->as.match.branches.items[i].body, arg_names, arena);
    }
  } break;

  case ExprKindSelf: break;
  }
}

static void clone_block(Exprs *block, Args *arg_names, Arena *arena) {
  Exprs new_block = {0};
  new_block.len = block->len;
  new_block.items = arena_alloc(arena, new_block.len * sizeof(Expr *));
  memcpy(new_block.items, block->items, new_block.len * sizeof(Expr *));

  for (u32 i = 0; i < new_block.len; ++i)
    clone_expr(new_block.items + i, arg_names, arena);

  *block = new_block;
}

static void rename_args_block(Exprs *block, Args *prev_arg_names,
                              Args *new_arg_names, Arena *arena);

static void rename_args_expr(Expr *expr, Args *prev_arg_names,
                             Args *new_arg_names, Arena *arena) {
  switch (expr->kind) {
  case ExprKindString: break;
  case ExprKindInt: break;
  case ExprKindFloat: break;
  case ExprKindBytes: break;

  case ExprKindBlock: {
    rename_args_block(&expr->as.block, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindIdent: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (expr->as.ident.name_id == prev_arg_names->items[i]) {
        expr->as.ident.name_id = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case ExprKindFunc: {
    for (u32 i = 0; i < expr->as.func.args.len; ++i) {
      for (u32 j = 0; j < prev_arg_names->len; ++j) {
        if (expr->as.func.args.items[i] == prev_arg_names->items[j]) {
          expr->as.func.args.items[i] = new_arg_names->items[j];

          break;
        }
      }
    }

    rename_args_block(&expr->as.func.body, prev_arg_names, new_arg_names, arena);
  } break;


  case ExprKindList: {
    rename_args_block(&expr->as.list.content, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindDict: {
    rename_args_block(&expr->as.dict.content, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindGet: {
    rename_args_block(&expr->as.get.chain, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindSet: {
    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (expr->as.set.name_id == prev_arg_names->items[i]) {
        expr->as.set.name_id = new_arg_names->items[i];

        break;
      }
    }

    rename_args_block(&expr->as.set.chain, prev_arg_names, new_arg_names, arena);
    rename_args_expr(expr->as.set.new, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindFuncCall: {
    rename_args_expr(expr->as.func_call.func, prev_arg_names, new_arg_names, arena);
    rename_args_block(&expr->as.func_call.args, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindLet: {
    rename_args_expr(expr->as.let.value, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < prev_arg_names->len; ++i) {
      if (expr->as.let.name_id == prev_arg_names->items[i]) {
        expr->as.let.name_id = new_arg_names->items[i];

        break;
      }
    }
  } break;

  case ExprKindRet: {
    if (expr->as.ret.value)
      rename_args_expr(expr->as.ret.value, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindIf: {
    rename_args_expr(expr->as._if.cond, prev_arg_names, new_arg_names, arena);
    rename_args_block(&expr->as._if.if_body, prev_arg_names, new_arg_names, arena);
    rename_args_block(&expr->as._if.else_body, prev_arg_names, new_arg_names, arena);
  } break;

  case ExprKindMatch: {
    rename_args_expr(expr->as.match.value, prev_arg_names, new_arg_names, arena);

    for (u32 i = 0; i < expr->as.match.branches.len; ++i) {
      rename_args_expr(expr->as.match.branches.items[i].value, prev_arg_names, new_arg_names, arena);
      rename_args_expr(expr->as.match.branches.items[i].body, prev_arg_names, new_arg_names, arena);
    }
  } break;

  case ExprKindSelf: break;
  }
}

static void rename_args_block(Exprs *block, Args *prev_arg_names,
                              Args *new_arg_names, Arena *arena) {
  for (u32 i = 0; i < block->len; ++i)
    rename_args_expr(block->items[i], prev_arg_names, new_arg_names, arena);
}

static void try_replace_macro_arg_ident(u16 *ident_id, Args *arg_names, Exprs *args) {
  u32 arg_index = get_macro_arg_index(*ident_id, arg_names);
  if (arg_index != (u32) -1 && args->items[arg_index]->kind == ExprKindIdent)
    *ident_id = args->items[arg_index]->as.ident.name_id;
}

static bool try_inline_macro_arg(Expr **expr, Args *arg_names,
                                 Exprs *args, DaExprs *dest,
                                 bool unpack, Arena *arena);

static void append_macro_arg(u32 index, Args *arg_names,
                             Exprs *args, DaExprs *dest,
                             bool unpack, Arena *arena) {
  Expr *arg = args->items[index];

  if (unpack && index + 1 == arg_names->len) {
    Exprs *variadic_args = &args->items[args->len - 1]->as.list.content;
    for (u32 i = 0; i < variadic_args->len; ++i) {
      Expr *new_arg = variadic_args->items[i];

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

static bool try_inline_macro_arg(Expr **expr, Args *arg_names,
                                 Exprs *args, DaExprs *dest,
                                 bool unpack, Arena *arena) {
  if (!arg_names || !args)
    return false;

  if ((*expr)->kind == ExprKindLet) {
    try_replace_macro_arg_ident(&(*expr)->as.let.name_id, arg_names, args);

    return false;
  }

  if ((*expr)->kind == ExprKindFunc) {
    for (u32 i = 0; i < (*expr)->as.func.args.len; ++i)
      try_replace_macro_arg_ident((*expr)->as.func.args.items + i, arg_names, args);

    return false;
  }

  if ((*expr)->kind == ExprKindIdent) {
    u32 arg_index = get_macro_arg_index((*expr)->as.ident.name_id, arg_names);
    if (arg_index != (u32) -1) {
      if (dest) {
        append_macro_arg(arg_index, arg_names, args, dest, unpack, arena);
      } else {
        Expr *arg = args->items[arg_index];
        clone_expr(&arg, arg_names, arena);
        *expr = arg;
      }

      return true;
    }
  }

  return false;
}

void expand_macros_block(Exprs *block, Macros *macros,
                         Args *arg_names, Exprs *args,
                         bool unpack, Arena *arena, Str *file_path,
                         i16 row, i16 col, bool is_inlined) {
  DaExprs new_block = {0};
  Da(bool) inlined_exprs = {0};

  for (u32 i = 0; i < block->len; ++i) {
    Expr *new_expr = block->items[i];

    bool inlined = try_inline_macro_arg(&new_expr, arg_names, args,
                                        &new_block, unpack, arena);
    if (!inlined) {
      clone_expr(&new_expr, arg_names, arena);
      block_append(&new_block, new_expr, arena);
    }

    for (u32 j = inlined_exprs.len; j < new_block.len; ++j)
      DA_APPEND(inlined_exprs, inlined);
  }

  for (u32 i = 0; i < new_block.len; ++i) {
    bool inlined = inlined_exprs.items[i];

    i16 temp_row = inlined ? 0 : row;
    i16 temp_col = inlined ? 0 : col;

    expand_macros(new_block.items[i], macros, arg_names,
                  args, unpack, arena, file_path,
                  temp_row, temp_col, is_inlined | inlined);
  }

  block->items = new_block.items;
  block->len = new_block.len;

  if (inlined_exprs.items)
    free(inlined_exprs.items);
}

void expand_macros(Expr *expr, Macros *macros,
                   Args *arg_names, Exprs *args,
                   bool unpack, Arena *arena, Str *file_path,
                   i16 row, i16 col, bool is_inlined) {
  switch (expr->kind) {
  case ExprKindString: break;
  case ExprKindInt: break;
  case ExprKindFloat: break;
  case ExprKindBytes: break;

  case ExprKindBlock: {
    INLINE_THEN_EXPAND_BLOCK(expr->as.block);
  } break;

  case ExprKindIdent: break;

  case ExprKindFunc: {
    INLINE_THEN_EXPAND_BLOCK(expr->as.func.body);
  } break;

  case ExprKindList: {
    INLINE_THEN_EXPAND_BLOCK(expr->as.list.content);
  } break;

  case ExprKindDict: {
    INLINE_THEN_EXPAND_BLOCK(expr->as.dict.content);
  } break;

  case ExprKindGet: {
    INLINE_THEN_EXPAND_BLOCK(expr->as.get.chain);
  } break;

  case ExprKindSet: {
    INLINE_THEN_EXPAND_BLOCK(expr->as.set.chain);
    INLINE_THEN_EXPAND(expr->as.set.new);
  } break;

  case ExprKindFuncCall: {
    INLINE_THEN_EXPAND(expr->as.func_call.func);
    INLINE_THEN_EXPAND_BLOCK(expr->as.func_call.args);

    if (expr->as.func_call.func->kind == ExprKindIdent) {
      u16 name_id = expr->as.func_call.func->as.ident.name_id;
      Macro *macro = get_macro(macros, name_id, expr->as.func_call.args.len);

      if (macro) {
        Exprs new_args = {
          expr->as.func_call.args.items,
          macro->arg_names.len,
        };

        if (macro->has_unpack) {
          --new_args.len;

          Exprs variadic;
          variadic.len = expr->as.func_call.args.len - new_args.len;
          variadic.items = arena_alloc(arena, variadic.len * sizeof(Expr *));

          for (u32 i = 0; i < variadic.len; ++i)
            variadic.items[i] = expr->as.func_call.args.items[new_args.len + i];

          Expr *variadic_args = arena_alloc(arena, sizeof(Expr));
          variadic_args->kind = ExprKindList;
          variadic_args->as.list.content.items = variadic.items;
          variadic_args->as.list.content.len = variadic.len;

          Expr **new_items = arena_alloc(arena, (new_args.len + 1) * sizeof(Expr *));
          memcpy(new_items, new_args.items, new_args.len * sizeof(Expr *));
          new_args.items = new_items;

          new_args.items[new_args.len++] = variadic_args;
        }

        Args new_arg_names = {0};
        StringBuilder sb = {0};

        Str name = get_str(macro->name_id);

        sb_push_str(&sb, name);
        sb_push_char(&sb, '@');

        u32 prev_len = sb.len;

        for (u32 i = 0; i < macro->arg_names.len; ++i) {
          Str arg_name = get_str(macro->arg_names.items[i]);
          sb_push_str(&sb, arg_name);

          u16 new_arg_name_id = copy_str(sb_to_str(sb));
          DA_APPEND(new_arg_names, new_arg_name_id);

          sb.len = prev_len;
        };

        free(sb.buffer);

        expr->kind = ExprKindBlock;
        expr->as.block = macro->body;

        Expr **new_items = arena_alloc(arena, expr->as.block.len * sizeof(Expr *));
        memcpy(new_items, expr->as.block.items, expr->as.block.len * sizeof(Expr *));
        expr->as.block.items = new_items;

        clone_block(&expr->as.block, &new_arg_names, arena);
        rename_args_block(&expr->as.block, &macro->arg_names,
                          &new_arg_names, arena);
        expand_macros_block(&expr->as.block, macros,
                            &new_arg_names, &new_args,
                            macro->has_unpack, arena,
                            expr->meta.file_path,
                            (i16) expr->meta.row - (i16) macro->row,
                            (i16) expr->meta.col - (i16) macro->col,
                            false);

        if (new_arg_names.items)
          free(new_arg_names.items);

        break;
      }
    }
  } break;

  case ExprKindLet: {
    INLINE_THEN_EXPAND(expr->as.let.value);
  } break;

  case ExprKindRet: {
    if (expr->as.ret.value)
      INLINE_THEN_EXPAND(expr->as.ret.value);
  } break;

  case ExprKindIf: {
    INLINE_THEN_EXPAND(expr->as._if.cond);
    INLINE_THEN_EXPAND_BLOCK(expr->as._if.if_body);
    INLINE_THEN_EXPAND_BLOCK(expr->as._if.else_body);
  } break;

  case ExprKindMatch: {
    INLINE_THEN_EXPAND(expr->as.match.value);

    for (u32 i = 0; i < expr->as.match.branches.len; ++i) {
      if (expr->as.match.branches.items[i].value)
        INLINE_THEN_EXPAND(expr->as.match.branches.items[i].value);
      INLINE_THEN_EXPAND(expr->as.match.branches.items[i].body);
    }
  } break;

  case ExprKindSelf: break;
  }

  if (arg_names && args && !is_inlined) {
    expr->meta.file_path = file_path;
    expr->meta.row = (i16) expr->meta.row + row;
    expr->meta.col = (i16) expr->meta.col + col;
  }
}
