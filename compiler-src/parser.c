#include "parser.h"
#include "lexgen/runtime-src/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl_log.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

#define MASK(id) (1 << (id))

typedef struct {
  u64 id;
  Str lexeme;
  u32 row, col;
} Token;

typedef Da(Token) Tokens;

typedef Da(Str) Strs;

typedef struct {
  Str     name;
  Strs    args;
  IrBlock body;
} Macro;

typedef Da(Macro) Macros;

typedef struct {
  Tokens tokens;
  u32    index;
  Ir     ir;
  Macros macros;
} Parser;

static char *token_names[] = {
  "whitespace",
  "new line",
  "comment",
  "fun",
  "let",
  "if",
  "else",
  "number",
  "identifier",
  "`(`",
  "`)`",
  "`[`",
  "`]`",
  "string literal",
  "`\\`",
};

static Tokens lex(Str code) {
  Tokens tokens = {0};
  TransitionTable *table = get_transition_table();
  u32 row = 0, col = 0;

  while (code.len > 0) {
    Token new_token = {0};

    new_token.lexeme = table_matches(table, &code, &new_token.id);
    new_token.row = row;
    new_token.col = col;

    if (new_token.id == (u64) -1) {
      ERROR("Unexpected `%c` at %u:%u\n", code.ptr[0], row + 1, col + 1);
      exit(1);
    }

    if (new_token.id == TT_STR) {
      bool is_escaped = false;
      while (code.len > 0 &&
             ((code.ptr[0] != '"' &&
               code.ptr[0] != '\'') ||
              is_escaped)) {
        if (is_escaped)
          is_escaped = false;
        else if (code.ptr[0] == '\\')
          is_escaped = true;

        ++new_token.lexeme.len;
        ++code.ptr;
        --code.len;
      }

      if (code.len == 0) {
        ERROR("String literal at %u:%u was not closed\n",
              new_token.row + 1, new_token.col + 1);
        exit(1);
      }

      ++new_token.lexeme.len;
      ++code.ptr;
      --code.len;
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

static Token *parser_expect_token(Parser *parser, u64 id_mask) {
  Token *token = parser_next_token(parser);
  if (!token || MASK(token->id) & id_mask)
    return token;

  u32 ids_count = 0;
  for (u32 i = 0; i < ARRAY_LEN(token_names); ++i)
    if (MASK(i) & id_mask)
      ++ids_count;

  ERROR("Expected ");

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

  if (token)
    fprintf(stderr, ", but got `"STR_FMT"` at %u:%u\n",
            STR_ARG(token->lexeme),
            token->row + 1, token->col + 1);
  else
    fprintf(stderr, ", but got EOF\n");

  exit(1);
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask);

static IrExprFuncDef parser_parse_func_def(Parser *parser) {
  IrExprFuncDef func_def = {0};

  parser_expect_token(parser, MASK(TT_FUN));

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  func_def.name = name_token->lexeme;

  parser_expect_token(parser, MASK(TT_OBRACKET));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT));
    DA_APPEND(func_def.args, arg_token->lexeme);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_CBRACKET));
  func_def.body = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  return func_def;
}

static IrExprLambda parser_parse_lambda(Parser *parser) {
  IrExprLambda lambda = {0};

  parser_expect_token(parser, MASK(TT_OPAREN));
  parser_expect_token(parser, MASK(TT_OBRACKET));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT));
    DA_APPEND(lambda.args, arg_token->lexeme);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_CBRACKET));
  lambda.body = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  return lambda;
}

static IrExprFuncCall parser_parse_func_call(Parser *parser) {
  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  IrBlock args = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));


  return (IrExprFuncCall) {
    name_token->lexeme,
    args,
  };
}

static void parser_parse_macro_def(Parser *parser) {
  Macro macro = {0};

  parser_expect_token(parser, MASK(TT_MACRO));

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  macro.name = name_token->lexeme;

  parser_expect_token(parser, MASK(TT_OBRACKET));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT));
    DA_APPEND(macro.args, arg_token->lexeme);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_CBRACKET));
  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(parser->macros, macro);
}

static void macro_body_expand_block(IrBlock *block, IrBlock *args, Strs arg_names);

static void macro_body_expand(IrExpr **expr, IrBlock *args, Strs arg_names) {
  IrExpr *new_expr = aalloc(sizeof(IrExpr));
  *new_expr = **expr;
  *expr = new_expr;

  switch (new_expr->kind) {
  case IrExprKindBlock: {
    macro_body_expand_block(&new_expr->as.block, args, arg_names);
  } break;

  case IrExprKindFuncDef: {
    macro_body_expand_block(&new_expr->as.func_def.body, args, arg_names);
  } break;

  case IrExprKindFuncCall: {
    macro_body_expand_block(&new_expr->as.func_call.args, args, arg_names);
  } break;

  case IrExprKindVarDef: {
    macro_body_expand(&new_expr->as.var_def.expr, args, arg_names);
  } break;

  case IrExprKindIf: {
    macro_body_expand_block(&new_expr->as._if.if_body, args, arg_names);
    if (new_expr->as._if.has_else)
      macro_body_expand_block(&new_expr->as._if.else_body, args, arg_names);
  } break;

  case IrExprKindList: {
    macro_body_expand_block(&new_expr->as.list.content, args, arg_names);
  } break;

  case IrExprKindIdent: {
    u32 index = (u32) -1;
    for (u32 i = 0; i < arg_names.len; ++i) {
      if (str_eq(new_expr->as.ident.ident, arg_names.items[i])) {
        index = i;
        break;
      }
    }

    if (index != (u32) -1)
      *new_expr = *args->items[index];
  } break;

  case IrExprKindLambda: {
    macro_body_expand_block(&new_expr->as.func_def.body, args, arg_names);
  } break;

  case IrExprKindStrLit: break;
  case IrExprKindNumber: break;
  case IrExprKindBool:   break;
  }
}

static void macro_body_expand_block(IrBlock *block, IrBlock *args, Strs arg_names) {
  for (u32 i = 0; i < block->len; ++i)
    macro_body_expand(block->items + i, args, arg_names);
}

static IrBlock parser_parse_macro_expand(Parser *parser) {
  Token *name_token = parser_expect_token(parser, MASK(TT_MACRO_NAME));
  Str name = name_token->lexeme;
  --name.len;

  IrBlock args = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  Macro *macro = NULL;

  for (u32 i = 0; i < parser->macros.len; ++i) {
    Macro *temp_macro = parser->macros.items + i;

    if (str_eq(temp_macro->name, name) &&
        temp_macro->args.len == args.len) {
      macro = temp_macro;
      break;
    }
  }

  if (!macro) {
    ERROR("Macro "STR_FMT" with %u arguments was not defined before usage\n",
          STR_ARG(name), args.len);
    exit(1);
  }

  IrBlock body = macro->body;
  macro_body_expand_block(&body, &args, macro->args);
  return body;
}

static IrExpr *parser_parse_expr(Parser *parser) {
  IrExpr *expr = aalloc(sizeof(IrExpr));

  Token *token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_OBRACKET) |
                                             MASK(TT_STR) | MASK(TT_IDENT) |
                                             MASK(TT_NUMBER) | MASK(TT_BOOL) |
                                             MASK(TT_SLASH));

  if (token->id == TT_STR) {
    expr->kind = IrExprKindStrLit;
    expr->as.str_lit.lit = STR(token->lexeme.ptr + 1,
                               token->lexeme.len - 2);

    return expr;
  }

  if (token->id == TT_IDENT) {
    expr->kind = IrExprKindIdent;
    expr->as.ident.ident = token->lexeme;

    return expr;
  }

  if (token->id == TT_NUMBER) {
    expr->kind = IrExprKindNumber;
    expr->as.number.number = str_to_i64(token->lexeme);

    return expr;
  }

  if (token->id == TT_BOOL) {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));

    return expr;
  }

  if (token->id == TT_OBRACKET) {
    expr->kind = IrExprKindList;
    expr->as.list.content = parser_parse_block(parser, MASK(TT_CBRACKET));
    parser_expect_token(parser, MASK(TT_CBRACKET));

    return expr;
  }

  if (token->id == TT_SLASH) {
    expr->kind = IrExprKindLambda;
    expr->as.lambda = parser_parse_lambda(parser);

    return expr;
  }

  token = parser_peek_token(parser);
  switch (token->id) {
  case TT_FUN: {
    expr->kind = IrExprKindFuncDef;
    expr->as.func_def = parser_parse_func_def(parser);
  } break;

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

    expr->as._if.if_body = parser_parse_block(parser, MASK(TT_CPAREN) | MASK(TT_ELSE));

    Token *next_token = parser_expect_token(parser, MASK(TT_CPAREN) | MASK(TT_ELSE));
    expr->as._if.has_else = next_token->id == TT_ELSE;
    if (expr->as._if.has_else) {
      expr->as._if.else_body = parser_parse_block(parser, MASK(TT_CPAREN));
      parser_expect_token(parser, MASK(TT_CPAREN));
    }
  } break;

  case TT_IDENT: {
    expr->kind = IrExprKindFuncCall;
    expr->as.func_call = parser_parse_func_call(parser);
  } break;

  case TT_MACRO_NAME: {
    expr->kind = IrExprKindBlock;
    expr->as.block = parser_parse_macro_expand(parser);
  } break;

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));
  } break;

  case TT_MACRO: {
    parser_parse_macro_def(parser);

    return NULL;
  } break;

  default: {
    parser_expect_token(parser, MASK(TT_FUN) | MASK(TT_LET) |
                                MASK(TT_IF) | MASK(TT_IDENT) |
                                MASK(TT_MACRO_NAME) | MASK(TT_BOOL) |
                                MASK(TT_MACRO));
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

Ir parse(Str code) {
  Parser parser = {0};

  parser.tokens = lex(code);
  parser.ir = parser_parse_block(&parser, 0);

  return parser.ir;
}
