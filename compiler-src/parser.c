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

typedef struct {
  Tokens tokens;
  u32    index;
  Ir     ir;
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

static IrExprFuncCall parser_parse_func_call(Parser *parser) {
  IrExprFuncCall func_call = {0};

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  func_call.name = name_token->lexeme;

  func_call.args = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  return func_call;
}

static IrExpr *parser_parse_expr(Parser *parser) {
  IrExpr *expr = aalloc(sizeof(IrExpr));

  Token *token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_OBRACKET) |
                                             MASK(TT_STR) | MASK(TT_IDENT) |
                                             MASK(TT_NUMBER) | MASK(TT_BOOL));

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

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));
  } break;

  default: {
    parser_expect_token(parser, MASK(TT_FUN) | MASK(TT_LET) |
                                MASK(TT_IF) | MASK(TT_IDENT));
  } break;
  }

  return expr;
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask) {
  IrBlock block = {0};

  Token *token = parser_peek_token(parser);
  while (token && !(MASK(token->id) & end_id_mask)) {
    IrExpr *expr = parser_parse_expr(parser);
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
