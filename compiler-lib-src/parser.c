#include "aether/parser.h"
#include "aether/macros.h"
#include "io.h"
#include "lexgen/runtime-src/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl/shl-log.h"

#define STD_PREFIX "/usr/include/aether/"
#define INCLUDE_PATHS(current_file_path) { current_file_path, STD_PREFIX }

#define MASK(id) (1 << (id))

typedef struct {
  u64   id;
  Str   lexeme;
  u32   row, col;
  char *file_path;
} Token;

typedef Da(Token) Tokens;

typedef struct {
  Tokens     tokens;
  u32        index;
  Macros    *macros;
  char      *file_path;
  FilePaths *included_files;
  Arena     *arena;
  Ir         ir;
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
  "get-at",
  "set-at",
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
  ":",
  "int",
  "float",
  "bool",
  "identifier",
};

static char escape_char(char _char) {
  switch (_char) {
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  case 'e': return '\e';
  case 'b': return '\b';
  case '0': return 0;
  case '1': return 1;
  case '2': return 2;
  case '3': return 3;
  case '4': return 4;
  case '5': return 5;
  case '6': return 6;
  case '7': return 7;
  case '8': return 8;
  case '9': return 9;
  default: return _char;
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

Ir parse_ex(Str code, char *file_path, Macros *macros,
            FilePaths *included_files, Arena *arena) {
  Parser parser = {0};

  parser.tokens = lex(code, file_path);
  parser.macros = macros;
  parser.included_files = included_files;
  parser.file_path = file_path;
  parser.arena = arena;
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
      DA_APPEND(macro.arg_names, arg_token->lexeme);

      break;
    }

    DA_APPEND(macro.arg_names, arg_token->lexeme);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_CBRACKET));
  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));

  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(*parser->macros, macro);
}

static IrExpr *parser_parse_expr(Parser *parser);

static IrExprDict parser_parse_dict(Parser *parser) {
  IrExprDict dict = {0};

  Token *token;
  while ((token = parser_peek_token(parser))->id != TT_CCURLY) {
    IrExpr *key = parser_parse_expr(parser);
    parser_expect_token(parser, MASK(TT_COLON));
    IrExpr *expr = parser_parse_expr(parser);

    IrField field = { key, expr };
    DA_APPEND(dict, field);
  }

  parser_expect_token(parser, MASK(TT_CCURLY));

  return dict;
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

    if (parser_peek_token(parser) &&
        parser_peek_token(parser)->id == TT_RHOMBUS)
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
  IrExpr *expr = arena_alloc(parser->arena, sizeof(IrExpr));
  *expr = (IrExpr) {0};

  Token *token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_STR) |
                                             MASK(TT_IDENT) | MASK(TT_INT) |
                                             MASK(TT_FLOAT) | MASK(TT_BOOL) |
                                             MASK(TT_OCURLY) | MASK(TT_OBRACKET) |
                                             MASK(TT_DOUBLE_ARROW));

  expr->meta.file_path = STR(token->file_path, strlen(token->file_path));
  expr->meta.row = token->row;
  expr->meta.col = token->col;

  switch (token->id) {
  case TT_STR: {
    expr->kind = IrExprKindString;
    expr->as.string.lit = STR(token->lexeme.ptr + 1,
                              token->lexeme.len - 2);

    return expr;
  };

  case TT_IDENT: {
    expr->kind = IrExprKindIdent;
    expr->as.ident.ident = token->lexeme;

    return expr;
  };

  case TT_INT: {
    expr->kind = IrExprKindInt;
    expr->as._int._int = str_to_i64(token->lexeme);

    return expr;
  };

  case TT_FLOAT: {
    expr->kind = IrExprKindFloat;
    expr->as._float._float = str_to_f64(token->lexeme);

    return expr;
  };

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));

    return expr;
  };

  case TT_OBRACKET: {
    u32 prev_index = parser->index;

    IrBlock block = parser_parse_block(parser, MASK(TT_CBRACKET));

    parser_expect_token(parser, MASK(TT_CBRACKET));

    if (parser_peek_token(parser)->id == TT_RIGHT_ARROW) {
      parser->index = prev_index;

      expr->kind = IrExprKindLambda;
      expr->as.lambda = parser_parse_lambda(parser);
    } else {
      expr->kind = IrExprKindList;
      expr->as.list.content = block;
    }

    return expr;
  };

  case TT_OCURLY: {
    expr->kind = IrExprKindDict;
    expr->as.dict = parser_parse_dict(parser);

    return expr;
  };

  case TT_DOUBLE_ARROW: {
    expr->kind = IrExprKindSelf;

    return expr;
  }
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

  case TT_USE: {
    parser_next_token(parser);

    Str current_file_path = {
      token->file_path,
      strlen(token->file_path),
    };

    Str new_file_path = parser_expect_token(parser, MASK(TT_STR))->lexeme;
    new_file_path.ptr += 1;
    new_file_path.len -= 2;

    parser_expect_token(parser, MASK(TT_CPAREN));

    char *prefix = str_to_cstr(get_file_dir(current_file_path));
    char *include_paths[] = INCLUDE_PATHS(prefix);
    char *path_cstr = NULL;
    Str path;
    Str code;
    bool already_included = false;

    for (u32 i = 0; i < ARRAY_LEN(include_paths); ++i) {
      if (path_cstr) {
        free(path_cstr);
        path_cstr = NULL;
      }

      StringBuilder path_sb = {0};
      sb_push(&path_sb, include_paths[i]);
      sb_push_str(&path_sb, new_file_path);
      path = sb_to_str(path_sb);
      path_cstr = str_to_cstr(path);

      already_included = false;
      for (u32 j = 0; j < parser->included_files->len; ++j) {
        if (str_eq(parser->included_files->items[j], path)) {
          already_included = true;

          break;
        }
      }

      if (already_included)
        break;

      code = read_file(path_cstr);

      if (code.len != (u32) -1) {
        DA_APPEND(*parser->included_files, path);

        break;
      }
    }

    if (already_included)
      break;

    if (code.len == (u32) -1) {
      ERROR("File "STR_FMT" was not found\n", STR_ARG(new_file_path));
      exit(1);
    }

    Macros macros = {0};

    expr->kind = IrExprKindBlock;
    expr->as.block = parse_ex(code, path_cstr, &macros,
                              parser->included_files,
                              parser->arena);

    if (parser->macros->cap < parser->macros->len + macros.len) {
      parser->macros->cap = parser->macros->len + macros.len;
      parser->macros->items = realloc(parser->macros->items,
                                      parser->macros->cap * sizeof(Macro));
    }

    memcpy(parser->macros->items + parser->macros->len,
           macros.items, macros.len * sizeof(Macro));
    parser->macros->len += macros.len;

    free(macros.items);
    free(prefix);
  } break;

  case TT_SET: {
    parser_next_token(parser);

    expr->kind = IrExprKindSet;
    expr->as.set.dest = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;
    expr->as.set.src = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_GET_AT: {
    parser_next_token(parser);

    expr->kind = IrExprKindGetAt;
    expr->as.get_at.src = parser_parse_expr(parser);
    expr->as.get_at.key = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_SET_AT: {
    parser_next_token(parser);

    expr->kind = IrExprKindSetAt;
    expr->as.set_at.dest = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;
    expr->as.set_at.key = parser_parse_expr(parser);
    expr->as.set_at.value = parser_parse_expr(parser);

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
      ir_block_append(&block, expr, parser->arena);

    token = parser_peek_token(parser);
  }

  return block;
}

Ir parse(Str code, char *file_path) {
  Macros macros = {0};
  FilePaths included_files = {0};
  Arena arena = {0};
  Ir ir = parse_ex(code, file_path, &macros, &included_files, &arena);
  expand_macros_block(&ir, &macros, NULL, NULL, false, &arena);

  return ir;
}
