#include "aether-compiler/parser.h"
#include "io.h"
#include "lexgen/runtime-src/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl_log.h"
#include "shl_arena.h"

#define MASK(id) (1 << (id))

typedef struct {
  u64   id;
  Str   lexeme;
  u32   row, col;
  char *file_path;
} Token;

typedef Da(Token) Tokens;

typedef struct {
  Str  name;
  bool is_expanded;
} MacroArg;

typedef Da(MacroArg) MacroArgs;

typedef struct {
  Str       name;
  MacroArgs args;
  IrBlock   body;
  bool      has_unpack;
} Macro;

typedef Da(Macro) Macros;

typedef struct {
  Tokens  tokens;
  u32     index;
  Macros *macros;
  char   *file_path;
  Ir      ir;
} Parser;

static char *token_names[] = {
  "whitespace",
  "new line",
  "comment",
  "let",
  "if",
  "elif",
  "else",
  "macro",
  "while",
  "use",
  "set",
  "field",
  "ret",
  "import",
  "`(`",
  "`)`",
  "`[`",
  "`]`",
  "`{`",
  "`}`",
  "string literal",
  "...",
  "->",
  "<->",
  "<>",
  "int",
  "float",
  "bool",
  "identifier",
  "key",
};

static char escape_char(char _char) {
  switch (_char) {
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  default:  return _char;
  }
}

static Tokens lex(Str code, char *file_path) {
  Tokens tokens = {0};
  TransitionTable *table = get_transition_table();
  u32 row = 0, col = 0;

  while (code.len > 0) {
    Token new_token = {0};
    new_token.lexeme = table_matches(table, &code, &new_token.id);
    new_token.row = row;
    new_token.col = col;
    new_token.file_path = file_path;

    if (new_token.id == (u64) -1) {
      PERROR("%s:%u:%u: ", "Unexpected `%c`\n",
             file_path, row + 1, col + 1, code.ptr[0]);
      exit(1);
    }

    if (new_token.id == TT_STR) {
      StringBuilder sb = {0};
      sb_push_char(&sb, code.ptr[0]);

      bool is_escaped = false;
      while (code.len > 0 &&
             ((code.ptr[0] != '"' &&
               code.ptr[0] != '\'') ||
              is_escaped)) {

        if (is_escaped || code.ptr[0] != '\\') {
          if (is_escaped)
            code.ptr[0] = escape_char(code.ptr[0]);
          sb_push_char(&sb, code.ptr[0]);
        }

        if (is_escaped)
          is_escaped = false;
        else if (code.ptr[0] == '\\')
          is_escaped = true;

        ++code.ptr;
        --code.len;
      }

      if (code.len == 0) {
        PERROR("%s:%u:%u: ", "String literal was not closed\n",
               new_token.file_path, new_token.row + 1, new_token.col + 1);
        exit(1);
      }

      ++code.ptr;
      --code.len;

      sb_push_char(&sb, code.ptr[0]);

      new_token.lexeme = sb_to_str(sb);
    }

    col += new_token.lexeme.len;
    if (new_token.id == TT_NEWLINE) {
      ++row;
      col = 0;
    } else if (new_token.id == TT_COMMENT) {
      while (code.len > 0 && code.ptr[0] != '\n') {
        ++code.ptr;
        --code.len;
      }
    } else if (new_token.id != TT_WHITESPACE) {
      DA_APPEND(tokens, new_token);
    }
  }

  return tokens;
}

static Token *parser_peek_token(Parser *parser) {
  if (parser->index >= parser->tokens.len)
    return NULL;
  return parser->tokens.items + parser->index;
}

static Token *parser_next_token(Parser *parser) {
  if (parser->index >= parser->tokens.len)
    return NULL;
  return parser->tokens.items + parser->index++;
}

static void print_id_mask(u64 id_mask) {
  u32 ids_count = 0;
  for (u32 i = 0; i < ARRAY_LEN(token_names); ++i)
    if (MASK(i) & id_mask)
      ++ids_count;

  for (u32 i = 0, j = 0; i < 64 && j < ids_count; ++i) {
    if ((1 << i) & id_mask) {
      if (j > 0) {
        if (j + 1 == ids_count)
          fputs(" or ", stderr);
        else
          fputs(", ", stderr);
      }

      fputs(token_names[i], stderr);

      ++j;
    }
  }
}

static Token *parser_expect_token(Parser *parser, u64 id_mask) {
  Token *token = parser_next_token(parser);
  if (!token) {
    PERROR("%s: ", "Expected ", parser->file_path);
    print_id_mask(id_mask);
    fprintf(stderr, ", but got EOF\n");
    exit(1);
  }

  if (MASK(token->id) & id_mask)
    return token;

  PERROR("%s:%u:%u: ", "Expected ",
         token->file_path,
         token->row + 1, token->col + 1);
  print_id_mask(id_mask);
  fprintf(stderr, ", but got `"STR_FMT"`\n",
          STR_ARG(token->lexeme));
  exit(1);
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask);

Ir parse_with_macros(Str code, char *file_path, Macros *macros) {
  Parser parser = {0};

  parser.tokens = lex(code, file_path);
  parser.macros = macros;
  parser.file_path = file_path;
  parser.ir = parser_parse_block(&parser, 0);

  return parser.ir;
}

static void parser_parse_macro_def(Parser *parser) {
  Macro macro = {0};

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  macro.name = name_token->lexeme;

  parser_expect_token(parser, MASK(TT_OBRACKET));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_UNPACK));
    if (arg_token->id == TT_UNPACK) {
      macro.has_unpack = true;

      arg_token = parser_expect_token(parser, MASK(TT_IDENT));
    }

    MacroArg arg = { arg_token->lexeme, false };
    DA_APPEND(macro.args, arg);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_CBRACKET));
  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));

  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(*parser->macros, macro);
}

static void macro_body_expand_block(IrBlock *block, Macro *macro, IrBlock *args);

static void macro_body_expand(IrExpr **expr, Macro *macro,
                              IrBlock *args, IrBlock *parent_block) {
  IrExpr *new_expr = aalloc(sizeof(IrExpr));
  *new_expr = **expr;
  *expr = new_expr;

  switch (new_expr->kind) {
  case IrExprKindBlock: {
    macro_body_expand_block(&new_expr->as.block, macro, args);
  } break;

  case IrExprKindFuncCall: {
    if (new_expr->as.func_call.func->kind == IrExprKindIdent)
      if (str_eq(new_expr->as.func_call.func->as.ident.ident, macro->name))
        break;

    macro_body_expand(&new_expr->as.func_call.func, macro, args, NULL);
    macro_body_expand_block(&new_expr->as.func_call.args, macro, args);
  } break;

  case IrExprKindVarDef: {
    macro_body_expand(&new_expr->as.var_def.expr, macro, args, NULL);
  } break;

  case IrExprKindIf: {
    macro_body_expand(&new_expr->as._if.cond, macro, args, NULL);

    macro_body_expand_block(&new_expr->as._if.if_body, macro, args);

    for (u32 i = 0; i < new_expr->as._if.elifs.len; ++i)
      macro_body_expand_block(&new_expr->as._if.elifs.items[i].body, macro, args);

    if (new_expr->as._if.has_else)
      macro_body_expand_block(&new_expr->as._if.else_body, macro, args);
  } break;

  case IrExprKindWhile: {
    macro_body_expand_block(&new_expr->as._while.body, macro, args);
  } break;

  case IrExprKindSet: {
    macro_body_expand(&new_expr->as.set.src, macro, args, NULL);
  } break;

  case IrExprKindField: {
    if (new_expr->as.field.is_set)
      macro_body_expand(&new_expr->as.field.expr, macro, args, NULL);
  } break;

  case IrExprKindRet: {
    if (new_expr->as.ret.has_expr)
      macro_body_expand(&new_expr->as.ret.expr, macro, args, NULL);
  } break;

  case IrExprKindList: {
    macro_body_expand_block(&new_expr->as.list.content, macro, args);
  } break;

  case IrExprKindIdent: {
    u32 index = (u32) -1;
    for (u32 i = 0; i < macro->args.len; ++i) {
      if (str_eq(new_expr->as.ident.ident, macro->args.items[i].name)) {
        index = i;
        break;
      }
    }

    if (index != (u32)- 1) {
      if (index + 1 == macro->args.len && macro->has_unpack) {
        if (!parent_block) {
          ERROR("Macro arguments can be unpacked only inside of a block\n");
          exit(1);
        }

        parent_block->len -= 1;

        IrBlock *block = &args->items[args->len - 1]->as.block;
        for (u32 i = 0; i < block->len; ++i) {
          DA_APPEND(*parent_block, block->items[i]);
          macro_body_expand(parent_block->items + parent_block->len - 1, macro, args, NULL);
        }
      } else if (!macro->args.items[index].is_expanded) {
        macro->args.items[index].is_expanded = true;

        *new_expr = *args->items[index];
        macro_body_expand(&new_expr, macro, args, NULL);

        macro->args.items[index].is_expanded = false;
      }
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt:    break;
  case IrExprKindFloat:  break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    macro_body_expand_block(&new_expr->as.lambda.body, macro, args);
  } break;

  case IrExprKindRecord: {
    for (u32 i = 0; new_expr->as.record.len; ++i)
      macro_body_expand(&new_expr->as.record.items[i].expr, macro, args, NULL);
  } break;

  case IrExprKindSelfCall: {
    for (u32 i = 0; new_expr->as.self_call.args.len; ++i)
      macro_body_expand(&new_expr->as.self_call.args.items[i], macro, args, NULL);
  } break;
  }
}

static void macro_body_expand_block(IrBlock *block, Macro *macro, IrBlock *args) {
  IrExpr **new_items = malloc(block->cap * sizeof(IrExpr *));
  memcpy(new_items, block->items, block->len * sizeof(IrExpr *));
  block->items = new_items;

  for (u32 i = 0; i < block->len; ++i)
    macro_body_expand(block->items + i, macro, args, block);
}

static Macro *get_macro(Parser *parser, Str name, u32 args_len) {
  for (u32 i = 0; i < parser->macros->len; ++i) {
    Macro *macro = parser->macros->items + i;

    if (str_eq(macro->name, name) &&
        (macro->args.len == args_len ||
         (macro->args.len <= args_len &&
          macro->has_unpack)))
      return macro;
  }

  return NULL;
}

static IrBlock parser_parse_macro_expand(Parser *parser, bool *found) {
  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  IrBlock args = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  Str name = name_token->lexeme;
  Macro *macro = get_macro(parser, name, args.len);
  *found = macro != NULL;
  if (!*found)
    return args;

  if (macro->has_unpack) {
    IrExpr *unpack_body = aalloc(sizeof(IrExpr));
    unpack_body->kind = IrExprKindBlock;
    for (u32 i = macro->args.len - 1; i < args.len; ++i)
      DA_APPEND(unpack_body->as.block, args.items[i]);

    args.len = macro->args.len;
    args.items[args.len - 1] = unpack_body;
  }

  IrBlock body = macro->body;
  macro_body_expand_block(&body, macro, &args);

  return body;
}

static IrExpr *parser_parse_expr(Parser *parser);

static IrExprRecord parser_parse_record(Parser *parser) {
  IrExprRecord record = {0};

  Token *token;
  while ((token = parser_peek_token(parser))->id != TT_CCURLY) {
    Token *key_token = parser_expect_token(parser, MASK(TT_KEY));
    Str key = key_token->lexeme;
    --key.len;

    IrExpr *expr = parser_parse_expr(parser);

    IrField field = { key, expr };
    DA_APPEND(record, field);
  }

  parser_expect_token(parser, MASK(TT_CCURLY));

  return record;
}

static IrExprLambda parser_parse_lambda(Parser *parser) {
  IrExprLambda lambda = {0};

  Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                                 MASK(TT_CBRACKET));
  while (arg_token->id != TT_CBRACKET) {
    DA_APPEND(lambda.args, arg_token->lexeme);
    arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                            MASK(TT_CBRACKET));
  }

  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  Token *token = parser_peek_token(parser);
  if (token && token->id == TT_IMPORT) {
    parser_next_token(parser);

    Token *intrinsic_name_token = parser_expect_token(parser, MASK(TT_STR));
    Str intrinsic_name = {
      intrinsic_name_token->lexeme.ptr + 1,
      intrinsic_name_token->lexeme.len - 2,
    };

    lambda.intrinsic_name = intrinsic_name;
  } else {
    lambda.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                             MASK(TT_CBRACKET) |
                                             MASK(TT_RHOMBUS));

    token = parser_peek_token(parser);
    if (token && token->id == TT_RHOMBUS)
      parser_next_token(parser);
  }

  return lambda;
}

static char *str_to_cstr(Str str) {
  char *result = malloc((str.len + 1) * sizeof(char));
  memcpy(result, str.ptr, str.len * sizeof(char));
  result[str.len] = 0;

  return result;
}

static Str get_file_dir(Str path) {
  for (u32 i = path.len; i > 0; --i)
    if (path.ptr[i - 1] == '/')
      return (Str) { path.ptr, i };

  return (Str) {0};
}

static IrExpr *parser_parse_expr(Parser *parser) {
  IrExpr *expr = aalloc(sizeof(IrExpr));
  *expr = (IrExpr) {0};

  Token *token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_STR) |
                                             MASK(TT_IDENT) | MASK(TT_INT) |
                                             MASK(TT_FLOAT) | MASK(TT_BOOL) |
                                             MASK(TT_OCURLY) | MASK(TT_OBRACKET));

  switch (token->id) {
  case TT_STR: {
    expr->kind = IrExprKindString;
    expr->as.string.lit = STR(token->lexeme.ptr + 1,
                              token->lexeme.len - 2);

    return expr;
  } break;

  case TT_IDENT: {
    expr->kind = IrExprKindIdent;
    expr->as.ident.ident = token->lexeme;

    return expr;
  } break;

  case TT_INT: {
    expr->kind = IrExprKindInt;
    expr->as._int._int = str_to_i64(token->lexeme);

    return expr;
  } break;

  case TT_FLOAT: {
    expr->kind = IrExprKindFloat;
    expr->as._float._float = str_to_f64(token->lexeme);

    return expr;
  } break;

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));

    return expr;
  } break;

  case TT_OBRACKET: {
    u32 index = parser->index;

    IrBlock block = parser_parse_block(parser, MASK(TT_CBRACKET));
    parser_expect_token(parser, MASK(TT_CBRACKET));

    token = parser_peek_token(parser);
    if (token->id == TT_RIGHT_ARROW) {
      parser->index = index;

      expr->kind = IrExprKindLambda;
      expr->as.lambda = parser_parse_lambda(parser);
    } else {
      expr->kind = IrExprKindList;
      expr->as.list.content = block;
    }

    return expr;
  } break;

  case TT_OCURLY: {
    expr->kind = IrExprKindRecord;
    expr->as.record = parser_parse_record(parser);

    return expr;
  } break;
  }

  token = parser_peek_token(parser);

  switch (token->id) {
  case TT_LET: {
    parser_next_token(parser);

    Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));

    expr->kind = IrExprKindVarDef;
    expr->as.var_def.name = name_token->lexeme;
    expr->as.var_def.expr = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_IF: {
    parser_next_token(parser);

    expr->kind = IrExprKindIf;
    expr->as._if.cond = parser_parse_expr(parser);

    expr->as._if.if_body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                                      MASK(TT_ELIF) |
                                                      MASK(TT_ELSE));

    Token *next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                                    MASK(TT_ELIF) |
                                                    MASK(TT_ELSE));

    while (next_token->id == TT_ELIF) {
      IrElif elif;
      elif.cond = parser_parse_expr(parser);
      elif.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                             MASK(TT_ELIF) |
                                             MASK(TT_ELSE));
      DA_APPEND(expr->as._if.elifs, elif);

      next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                               MASK(TT_ELIF) |
                                               MASK(TT_ELSE));
    }

    expr->as._if.has_else = next_token->id == TT_ELSE;
    if (expr->as._if.has_else) {
      expr->as._if.else_body = parser_parse_block(parser, MASK(TT_CPAREN));
      parser_expect_token(parser, MASK(TT_CPAREN));
    }
  } break;

  case TT_MACRO: {
    parser_next_token(parser);

    parser_parse_macro_def(parser);

    return NULL;
  } break;

  case TT_WHILE: {
    parser_next_token(parser);

    expr->kind = IrExprKindWhile;
    expr->as._while.cond = parser_parse_expr(parser);
    expr->as._while.body = parser_parse_block(parser, MASK(TT_CPAREN));

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_SET: {
    parser_next_token(parser);

    expr->kind = IrExprKindSet;
    expr->as.set.dest = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;
    expr->as.set.src = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_USE: {
    parser_next_token(parser);

    Str current_file_path = {
      token->file_path,
      strlen(token->file_path),
    };

    Str new_file_path = parser_expect_token(parser, MASK(TT_STR))->lexeme;
    new_file_path.ptr += 1;
    new_file_path.len -= 2;

    StringBuilder path_sb = {0};
    sb_push_str(&path_sb, get_file_dir(current_file_path));
    sb_push_str(&path_sb, new_file_path);
    char *path = str_to_cstr(sb_to_str(path_sb));

    Str code = read_file(path);
    if (code.len == (u32) -1) {
      ERROR("File %s was not found\n", path);
      exit(1);
    }

    expr->kind = IrExprKindBlock;
    expr->as.block = parse_with_macros(code, path, parser->macros);

    parser_expect_token(parser, MASK(TT_CPAREN));

    free(path_sb.buffer);
  } break;

  case TT_FIELD: {
    parser_next_token(parser);

    expr->kind = IrExprKindField;
    expr->as.field.record = parser_parse_expr(parser);
    expr->as.field.field = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;

    if (parser_peek_token(parser)->id != TT_CPAREN) {
      expr->as.field.is_set = true;
      expr->as.field.expr = parser_parse_expr(parser);
    }

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_RET: {
    parser_next_token(parser);

    expr->kind = IrExprKindRet;
    expr->as.ret.has_expr = parser_peek_token(parser)->id != TT_CPAREN;

    if (expr->as.ret.has_expr)
      expr->as.ret.expr = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  default: {
    token = parser_peek_token(parser);

    if (token && token->id == TT_DOUBLE_ARROW) {
      parser_next_token(parser);

      expr->kind = IrExprKindSelfCall;
      expr->as.self_call.args = parser_parse_block(parser, MASK(TT_CPAREN));

      parser_expect_token(parser, MASK(TT_CPAREN));

      break;
    }

    if (token && token->id == TT_IDENT) {
      u32 prev_parser_index = parser->index;
      bool found_macro = false;
      IrBlock args = parser_parse_macro_expand(parser, &found_macro);

      if (found_macro) {
        expr->kind = IrExprKindBlock;
        expr->as.block = args;

        break;
      }

      parser->index = prev_parser_index;
    }

    expr->kind = IrExprKindFuncCall;
    expr->as.func_call.func = parser_parse_expr(parser);
    expr->as.func_call.args = parser_parse_block(parser, MASK(TT_CPAREN));

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;
  }

  return expr;
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask) {
  IrBlock block = {0};

  Token *token = parser_peek_token(parser);
  while (token && !(MASK(token->id) & end_id_mask)) {
    IrExpr *expr = parser_parse_expr(parser);
    if (expr)
      DA_APPEND(block, expr);

    token = parser_peek_token(parser);
  }

  return block;
}

Ir parse(Str code, char *file_path) {
  Macros macros = {0};
  return parse_with_macros(code, file_path, &macros);
}
