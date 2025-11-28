#define SHL_DEFS_LL_ALLOC(size) arena_alloc(arena, size)

#include "aether/parser.h"
#include "aether/macros.h"
#include "io.h"
#include "lexgen/runtime-src/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl/shl-log.h"

#define STD_PREFIX "/usr/include/aether/"

#ifdef __emscripten__
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

#define EMSCRIPTEN_STD_PREFIX "dest/"
#define INCLUDE_PATHS(current_file_path) { current_file_path, STD_PREFIX, EMSCRIPTEN_STD_PREFIX }
#else
#define INCLUDE_PATHS(current_file_path) { current_file_path, STD_PREFIX }
#endif

#define MASK(id) (1 << (id))

typedef struct Token Token;

struct Token {
  u64    id;
  Str    lexeme;
  u32    row, col;
  char  *file_path;
  Token *next;
};

typedef struct {
  Token     *tokens;
  Token     *current_token;
  Macros    *macros;
  char      *file_path;
  FilePaths *included_files;
  Arena     *arena;
  Arena     *persistent_arena;
  bool       is_in_macro;
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
  "ret",
  "import",
  "match",
  "`(`",
  "`)`",
  "`[`",
  "`]`",
  "`{`",
  "`}`",
  "string literal",
  "`...`",
  "`->`",
  "`<>`",
  "`:`",
  "`::`"
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

static Token *lex(Str code, char *file_path, Arena *arena) {
  Token *tokens = NULL;
  Token *tokens_end = NULL;
  LL_PREPEND(tokens, tokens_end, Token);

  TransitionTable *table = get_transition_table();
  u32 row = 0, col = 0;

  while (code.len > 0) {
    u64 id = 0;
    Str lexeme = table_matches(table, &code, &id);

    if (id == TT_NEWLINE) {
      ++row;
      col = 0;

      continue;
    } else if (id == TT_COMMENT) {
      while (code.len > 0 && code.ptr[0] != '\n') {
        ++code.ptr;
        --code.len;
      }

      col += lexeme.len;
      continue;
    } else if (id == TT_WHITESPACE) {
      col += lexeme.len;
      continue;
    }

    LL_PREPEND(tokens, tokens_end, Token);
    tokens_end->id = id;
    tokens_end->lexeme = lexeme;
    tokens_end->row = row;
    tokens_end->col = col;
    tokens_end->file_path = file_path;

    if (tokens_end->id == (u64) -1) {
      PERROR("%s:%u:%u: ", "Unexpected `%c`\n",
             file_path, row + 1, col + 1, code.ptr[0]);
      exit(1);
    }

    if (tokens_end->id == TT_STR) {
      StringBuilder sb = {0};
      sb_push_char(&sb, code.ptr[-1]);

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
        ++col;
      }

      if (code.len == 0) {
        PERROR("%s:%u:%u: ", "String literal was not closed\n",
               tokens_end->file_path, tokens_end->row + 1, tokens_end->col + 1);
        exit(1);
      }

      sb_push_char(&sb, code.ptr[0]);

      ++code.ptr;
      --code.len;
      ++col;

      tokens_end->lexeme.len = sb.len;
      tokens_end->lexeme.ptr = arena_alloc(arena, tokens_end->lexeme.len);
      memcpy(tokens_end->lexeme.ptr, sb.buffer, tokens_end->lexeme.len);

      free(sb.buffer);
    } else {
      col += tokens_end->lexeme.len;
    }
  }

  return tokens;
}

static Token *parser_peek_token(Parser *parser) {
  if (parser->current_token)
    return parser->current_token->next;
  return NULL;
}

static Token *parser_next_token(Parser *parser) {
  if (parser->current_token) {
    parser->current_token = parser->current_token->next;
    return parser->current_token;
  }
  return NULL;
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
            FilePaths *included_files, Arena *arena,
            Arena *persistent_arena) {
  Parser parser = {0};

  parser.tokens = lex(code, file_path, arena);
  parser.current_token = parser.tokens;
  parser.macros = macros;
  parser.included_files = included_files;
  parser.file_path = file_path;
  parser.arena = arena;
  parser.persistent_arena = persistent_arena;
  parser.ir = parser_parse_block(&parser, 0);

  return parser.ir;
}

static Str copy_str(Str str, Arena *arena) {
  Str copy;

  copy.len = str.len;
  copy.ptr = arena_alloc(arena, str.len);
  memcpy(copy.ptr, str.ptr, copy.len);

  return copy;
}

static Arena *get_arena(Parser *parser) {
  if (parser->is_in_macro)
    return parser->persistent_arena;
  return parser->arena;
}

static void parser_parse_macro_def(Parser *parser) {
  Macro macro = {0};

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  macro.name = copy_str(name_token->lexeme, parser->persistent_arena);

  parser_expect_token(parser, MASK(TT_OBRACKET));

  IrArgs arg_names = {0};

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_UNPACK));
    if (arg_token->id == TT_UNPACK) {
      macro.has_unpack = true;

      arg_token = parser_expect_token(parser, MASK(TT_IDENT));
      Str arg_name = copy_str(arg_token->lexeme, parser->persistent_arena);
      DA_APPEND(arg_names, arg_name);

      break;
    }

    Str arg_name = copy_str(arg_token->lexeme, parser->persistent_arena);
    DA_APPEND(arg_names, arg_name);

    next_token = parser_peek_token(parser);
  }

  macro.arg_names.len = arg_names.len;
  macro.arg_names.cap = macro.arg_names.len;
  macro.arg_names.items = arena_alloc(parser->persistent_arena, macro.arg_names.cap * sizeof(Str));
  memcpy(macro.arg_names.items, arg_names.items, macro.arg_names.len * sizeof(Str));

  free(arg_names.items);

  parser_expect_token(parser, MASK(TT_CBRACKET));
  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  bool prev_is_in_macro = parser->is_in_macro;
  parser->is_in_macro = true;

  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));

  parser->is_in_macro = prev_is_in_macro;

  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(*parser->macros, macro);
}

static IrExpr *parser_parse_expr(Parser *parser);

static IrExprDict parser_parse_dict(Parser *parser) {
  IrExprDict dict = {0};

  IrFields fields = {0};

  while (parser_peek_token(parser)->id != TT_CCURLY) {
    IrExpr *key = parser_parse_expr(parser);
    parser_expect_token(parser, MASK(TT_COLON));
    IrExpr *expr = parser_parse_expr(parser);

    IrField field = { key, expr };
    DA_APPEND(fields, field);
  }

  dict.len = fields.len;
  dict.cap = dict.len;
  dict.items = arena_alloc(get_arena(parser), dict.cap * sizeof(IrField));
  memcpy(dict.items, fields.items, dict.len * sizeof(IrField));

  free(fields.items);

  parser_expect_token(parser, MASK(TT_CCURLY));

  return dict;
}

static IrExprLambda parser_parse_lambda(Parser *parser) {
  IrExprLambda lambda = {0};

  IrArgs args = {0};

  Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                                 MASK(TT_CBRACKET));
  while (arg_token->id != TT_CBRACKET) {
    DA_APPEND(args, arg_token->lexeme);
    arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                            MASK(TT_CBRACKET));
  }

  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  lambda.args.len = args.len;
  lambda.args.cap = lambda.args.len;
  lambda.args.items = arena_alloc(get_arena(parser), lambda.args.cap * sizeof(Str));
  memcpy(lambda.args.items, args.items, args.len * sizeof(Str));

  free(args.items);

  Token *token = parser_peek_token(parser);
  if (token && token->id == TT_IMPORT) {
    parser_next_token(parser);

    Token *intrinsic_name_token = parser_expect_token(parser, MASK(TT_STR));
    Str intrinsic_name = {
      intrinsic_name_token->lexeme.ptr + 1,
      intrinsic_name_token->lexeme.len - 2,
    };
    lambda.intrinsic_name = copy_str(intrinsic_name, parser->persistent_arena);
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

static IrExprMatch parser_parse_match(Parser *parser) {
  IrExprMatch match = {0};

  match.src = parser_parse_expr(parser);

  IrCases cases = {0};

  while (parser_peek_token(parser)->id != TT_CPAREN) {
    IrExpr *pattern = parser_parse_expr(parser);
    parser_expect_token(parser, MASK(TT_RIGHT_ARROW));
    IrExpr *expr = parser_parse_expr(parser);

    IrCase _case = { pattern, expr };
    DA_APPEND(cases, _case);
  }

  match.cases.len = cases.len;
  match.cases.cap = match.cases.len;
  match.cases.items = arena_alloc(get_arena(parser), match.cases.cap * sizeof(IrCase));
  memcpy(match.cases.items, cases.items, match.cases.len * sizeof(IrCase));

  free(cases.items);

  parser_expect_token(parser, MASK(TT_CPAREN));

  return match;
}

static char *str_to_cstr(Str str) {
  char *result = malloc((str.len + 1) * sizeof(char));
  memcpy(result, str.ptr, str.len);
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
  IrExpr *expr = arena_alloc(get_arena(parser), sizeof(IrExpr));
  *expr = (IrExpr) {0};

  Token *token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_STR) |
                                             MASK(TT_IDENT) | MASK(TT_INT) |
                                             MASK(TT_FLOAT) | MASK(TT_BOOL) |
                                             MASK(TT_OCURLY) | MASK(TT_OBRACKET));

  expr->meta.file_path = copy_str(STR(token->file_path, strlen(token->file_path)),
                                  parser->persistent_arena);

  expr->meta.row = token->row;
  expr->meta.col = token->col;

  bool found_atom = true;

  switch (token->id) {
  case TT_STR: {
    expr->kind = IrExprKindString;

    expr->as.string.lit = copy_str(STR(token->lexeme.ptr + 1, token->lexeme.len - 2),
                                   parser->persistent_arena);
  } break;

  case TT_IDENT: {
    expr->kind = IrExprKindIdent;
    expr->as.ident.ident = copy_str(token->lexeme, parser->persistent_arena);
  } break;

  case TT_INT: {
    expr->kind = IrExprKindInt;
    expr->as._int._int = str_to_i64(token->lexeme);
  } break;

  case TT_FLOAT: {
    expr->kind = IrExprKindFloat;
    expr->as._float._float = str_to_f64(token->lexeme);
  } break;

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));
  } break;

  case TT_OBRACKET: {
    Token *prev_token = parser->current_token;

    IrBlock block = parser_parse_block(parser, MASK(TT_CBRACKET));

    parser_expect_token(parser, MASK(TT_CBRACKET));

    if (parser_peek_token(parser)->id == TT_RIGHT_ARROW) {
      parser->current_token = prev_token;

      expr->kind = IrExprKindLambda;
      expr->as.lambda = parser_parse_lambda(parser);
    } else {
      expr->kind = IrExprKindList;
      expr->as.list.content = block;
    }
  } break;

  case TT_OCURLY: {
    expr->kind = IrExprKindDict;
    expr->as.dict = parser_parse_dict(parser);
  } break;

  default: {
    found_atom = false;
  } break;
  }

  if (!found_atom) {
    token = parser_peek_token(parser);

    switch (token->id) {
    case TT_LET: {
      parser_next_token(parser);

      Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));

      expr->kind = IrExprKindVarDef;
      expr->as.var_def.name = copy_str(name_token->lexeme, parser->persistent_arena);
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

      IrElifs elifs = {0};

      while (next_token->id == TT_ELIF) {
        IrElif elif;
        elif.cond = parser_parse_expr(parser);
        elif.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                               MASK(TT_ELIF) |
                                               MASK(TT_ELSE));
        DA_APPEND(elifs, elif);

        next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                                 MASK(TT_ELIF) |
                                                 MASK(TT_ELSE));
      }

      expr->as._if.elifs.len = elifs.len;
      expr->as._if.elifs.cap = expr->as._if.elifs.len;
      expr->as._if.elifs.items =
        arena_alloc(get_arena(parser), expr->as._if.elifs.cap * sizeof(IrElif));
      memcpy(expr->as._if.elifs.items, elifs.items, expr->as._if.elifs.len * sizeof(IrElif));

      free(elifs.items);

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
      Str path = {0};
      Str code = { NULL, (u32) -1 };
      bool already_included = false;

      for (u32 i = 0; i < ARRAY_LEN(include_paths); ++i) {
        if (path.ptr)
          free(path.ptr);

        StringBuilder path_sb = {0};
        sb_push(&path_sb, include_paths[i]);
        sb_push_str(&path_sb, new_file_path);
        sb_push_char(&path_sb, '\0');
        path = (Str) { path_sb.buffer, path_sb.len };

        already_included = false;
        for (u32 j = 0; j < parser->included_files->len; ++j) {
          if (str_eq(parser->included_files->items[j], path)) {
            already_included = true;
            free(path.ptr);
            break;
          }
        }

        if (already_included)
          break;

        code = read_file_arena(path.ptr, get_arena(parser));

        if (code.len != (u32) -1) {
          char *new_ptr = arena_alloc(get_arena(parser), path.len);
          memcpy(new_ptr, path.ptr, path.len);
          free(path.ptr);
          path.ptr = new_ptr;

          DA_APPEND(*parser->included_files, path);

          break;
        }
      }

      if (already_included) {
        free(prefix);
        break;
      }

      if (code.len == (u32) -1) {
        PERROR("%s:%u:%u: ", "File "STR_FMT" was not found\n",
               token->file_path, token->row + 1, token->col + 1,
               STR_ARG(new_file_path));
        exit(1);
      }

      expr->kind = IrExprKindBlock;
      expr->as.block = parse_ex(code, path.ptr, parser->macros,
                                parser->included_files, parser->arena,
                                parser->persistent_arena);

      free(prefix);
    } break;

    case TT_SET: {
      parser_next_token(parser);

      Str dest = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;

      token = parser_peek_token(parser);
      if (token->id == TT_QOLON) {
        parser_next_token(parser);

        expr->kind = IrExprKindSetAt;
        expr->as.set_at.dest = dest;
        expr->as.set_at.key = parser_parse_expr(parser);
        expr->as.set_at.value = parser_parse_expr(parser);
      } else {
        expr->kind = IrExprKindSet;
        expr->as.set.dest = dest;
        expr->as.set.src = parser_parse_expr(parser);
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

    case TT_MATCH: {
      parser_next_token(parser);

      expr->kind = IrExprKindMatch;
      expr->as.match = parser_parse_match(parser);
    } break;

    default: {
      expr->kind = IrExprKindFuncCall;
      expr->as.func_call.func = parser_parse_expr(parser);
      expr->as.func_call.args = parser_parse_block(parser, MASK(TT_CPAREN));

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;
    }
  }

  token = parser_peek_token(parser);
  if (token && token->id == TT_QOLON) {
    parser_next_token(parser);

    IrExpr *get_at = arena_alloc(get_arena(parser), sizeof(IrExpr));
    get_at->kind = IrExprKindGetAt;
    get_at->as.get_at.src = expr;
    get_at->as.get_at.key = parser_parse_expr(parser);

    get_at->meta.file_path = STR(token->file_path, strlen(token->file_path));
    get_at->meta.row = token->row;
    get_at->meta.col = token->col;

    expr = get_at;
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

Ir parse(Str code, char *file_path, Arena *arena, Arena *persistent_arena) {
  Macros macros = {0};
  FilePaths included_files = {0};
  Ir ir = parse_ex(code, file_path, &macros, &included_files,
                   arena, persistent_arena);
  expand_macros_block(&ir, &macros, NULL, NULL, false, persistent_arena);

  if (macros.items)
    free(macros.items);
  if (included_files.items)
    free(included_files.items);

  return ir;
}
