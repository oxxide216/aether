#include <wchar.h>
#include <unistd.h>

#define SHL_DEFS_LL_ALLOC(size) arena_alloc(arena, size)

#include "aether/parser.h"
#include "aether/macros.h"
#include "aether/common.h"
#include "aether/deserializer.h"
#include "aether/io.h"
#include "lexgen/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl/shl-log.h"

#define STD_PREFIX_CT "ae-src/"
#define STD_PREFIX_RT "/usr/include/aether/"

#define INCLUDE_PATHS { NULL,           \
                        STD_PREFIX_CT,  \
                        STD_PREFIX_RT }
#define INCLUDE_PATHS_LEN 3

#define MASK(id) (1 << (id))

typedef enum {
  TokenStatusOk = 0,
  TokenStatusEmpty,
  TokenStatusEOF,
} TokenStatus;

typedef struct {
  u64   id;
  Str   lexeme;
  Str  *file_path;
  u16   row, col;
  bool  eof;
} Token;

typedef struct {
  Str              code;
  u32              row, col;
  TransitionTable *table;
  StringBuilder    temp_sb;
} Lexer;

typedef struct {
  Lexer      lexer;
  Macros    *macros;
  Str       *file_path;
  FilePaths *included_files;
  Arena     *arena;
  bool       use_macros;
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
  "set",
  "use",
  "ret",
  "import",
  "match",
  "do",
  "`(`",
  "`)`",
  "`[`",
  "`]`",
  "`{`",
  "`}`",
  "string literal",
  "`...`",
  "`->`",
  "`:`",
  "`::`",
  "`<>`",
  "`<->`",
  "int",
  "float",
  "bool",
  "identifier",
};

CachedIrs cached_irs = {0};

static char escape_char(Str *str, u32 *col) {
  char _char = str->ptr[0];

  switch (_char) {
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  case 'e': return '\e';
  case 'b': return '\b';
  case '0': return 0;
  case '\\': return '\\';

  case 'x': {
    char result = '\0';

    ++str->ptr;
    --str->len;
    ++*col;

    while (str->len > 0 &&
           ((str->ptr[0] >= '0' && str->ptr[0] <= '9') ||
            (str->ptr[0] >= 'a' && str->ptr[0] <= 'f') ||
            (str->ptr[0] >= 'A' && str->ptr[0] <= 'F'))) {
      result *= 16;

      if (str->ptr[0] >= '0' && str->ptr[0] <= '9')
        result += str->ptr[0] - '0';
      else if (str->ptr[0] >= 'a' && str->ptr[0] <= 'f')
        result += str->ptr[0] - 'a' + 10;
      else if (str->ptr[0] >= 'A' && str->ptr[0] <= 'F')
        result += str->ptr[0] - 'A' + 10;

      ++str->ptr;
      --str->len;
    }

    --str->ptr;
    ++str->len;
    ++*col;

    return result;
  }

  case 'd': {
    char result = '\0';

    ++str->ptr;
    --str->len;
    ++*col;

    while (str->len > 0 && str->ptr[0] >= '0' && str->ptr[0] <= '9') {
      result *= 10;

      if (str->ptr[0] >= '0' && str->ptr[0] <= '9')
        result += str->ptr[0] - '0';

      ++str->ptr;
      --str->len;
      ++*col;
    }

    --str->ptr;
    ++str->len;
    ++*col;

    return result;
  }

  case 'o': {
    char result = '\0';

    ++str->ptr;
    --str->len;
    ++*col;

    while (str->len > 0 && str->ptr[0] >= '0' && str->ptr[0] <= '7') {
      result *= 8;

      if (str->ptr[0] >= '0' && str->ptr[0] <= '7')
        result += str->ptr[0] - '0';

      ++str->ptr;
      --str->len;
      ++*col;
    }

    --str->ptr;
    ++str->len;
    ++*col;

    return result;
  }

  default: return _char;
  }
}

static TokenStatus lex(Lexer *lexer, Token *token, Str *file_path, Arena *arena) {
  if (lexer->code.len > 0) {
    u64 id = 0;
    u32 char_len;
    Str lexeme = table_matches(lexer->table, &lexer->code, &id, &char_len);
    u16 row = lexer->row;
    u16 col = lexer->col;

    if (id == TT_NEWLINE) {
      ++lexer->row;
      lexer->col = 0;

      return TokenStatusEmpty;
    } else if (id == TT_COMMENT) {
      u32 next_len;
      wchar next;

      while ((next = get_next_wchar(lexer->code, 0, &next_len)) != U'\0' &&
             next != U'\n') {
        lexer->code.ptr += next_len;
        lexer->code.len -= next_len;
      }

      return TokenStatusEmpty;
    } else if (id == TT_WHITESPACE) {
      lexer->col += char_len;

      return TokenStatusEmpty;
    }

    if (id == (u64) -1) {
      u32 wchar_len;
      wchar _wchar = get_next_wchar(lexer->code, 0, &wchar_len);

      PERROR(STR_FMT":%u:%u: ", "Unexpected `%lc`\n", STR_ARG(*file_path),
             lexer->row + 1, lexer->col + 1, (wint_t) _wchar);
      exit(1);
    }

    if (id == TT_STR) {
      sb_push_char(&lexer->temp_sb, lexer->code.ptr[-1]);

      bool is_escaped = false;
      while (lexer->code.len > 0 &&
             (lexer->code.ptr[0] != lexer->temp_sb.buffer[0] ||
              is_escaped)) {
        u32 next_len;
        wchar next = get_next_wchar(lexer->code, 0, &next_len);

        if (is_escaped || next != U'\\') {
          if (is_escaped) {
            sb_push_char(&lexer->temp_sb, escape_char(&lexer->code, &lexer->col));
          } else {
            for (u32 i = 0; i < next_len; ++i)
              sb_push_char(&lexer->temp_sb, lexer->code.ptr[i]);
          }
        }

        if (is_escaped)
          is_escaped = false;
        else if (next == U'\\')
          is_escaped = true;

        lexer->code.ptr += next_len;
        lexer->code.len -= next_len;
        ++lexer->col;
      }

      if (lexer->code.len == 0) {
        PERROR(STR_FMT":%u:%u: ", "String literal was not closed\n",
               STR_ARG(*file_path), row + 1, col + 1);
        exit(1);
      }

      sb_push_char(&lexer->temp_sb, lexer->code.ptr[0]);

      ++lexer->code.ptr;
      --lexer->code.len;
      ++lexer->col;

      lexeme = copy_str(STR(lexer->temp_sb.buffer, lexer->temp_sb.len), arena);

      lexer->temp_sb.len = 0;
    } else {
      lexer->col += char_len;
    }

    *token = (Token) { id, lexeme, file_path, row, col, false };

    return TokenStatusOk;
  }

  return TokenStatusEOF;
}

static void load_lexer(Parser *parser, Lexer *lexer) {
  parser->lexer.code = lexer->code;
  parser->lexer.row = lexer->row;
  parser->lexer.col = lexer->col;
}

static Token parser_next_token(Parser *parser) {
  Token token;
  TokenStatus status;
  while ((status = lex(&parser->lexer, &token,
                       parser->file_path, parser->arena)) == TokenStatusEmpty);

  if (status == TokenStatusEOF)
    token.eof = true;

  return token;
}

static Token parser_peek_token(Parser *parser) {
  Lexer prev_lexer = parser->lexer;

  Token token = parser_next_token(parser);

  load_lexer(parser, &prev_lexer);

  return token;
}

static void print_id_mask(u64 id_mask) {
  u32 ids_count = 0;
  for (u64 i = 0; i < ARRAY_LEN(token_names); ++i)
    if (MASK(i) & id_mask)
      ++ids_count;

  for (u64 i = 0, j = 0; i < 64 && j < ids_count; ++i) {
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

static Token parser_expect_token(Parser *parser, u64 id_mask) {
  Token token = parser_next_token(parser);
  if (token.eof) {
    PERROR(STR_FMT": ", "Expected ", STR_ARG(*parser->file_path));
    print_id_mask(id_mask);
    fprintf(stderr, ", but got EOF\n");
    exit(1);
  }

  if (MASK(token.id) & id_mask)
    return token;

  PERROR(STR_FMT":%u:%u: ", "Expected ",
         STR_ARG(*token.file_path),
         token.row + 1, token.col + 1);
  print_id_mask(id_mask);
  fprintf(stderr, ", but got `"STR_FMT"`\n",
          STR_ARG(token.lexeme));
  exit(1);
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask);

Ir parse_ex(Str code, Str *file_path, Macros *macros,
            FilePaths *included_files, Arena *arena,
            bool use_macros) {
  for (u32 i = 0; i < cached_irs.len; ++i) {
    CachedIr *cached_ir = cached_irs.items + i;

    if (str_eq(*cached_ir->file_path, *file_path)) {
      DA_EXTEND(*macros, cached_ir->macros);
      DA_EXTEND(*included_files, cached_ir->included_files);
      *arena = cached_ir->arena;

      return cached_ir->ir;
    }
  }

  DA_APPEND(*included_files, file_path);

  Parser parser = {0};

  parser.lexer.code = code;
  parser.lexer.table = get_transition_table();
  parser.macros = macros;
  parser.file_path = file_path;
  parser.included_files = included_files;
  parser.arena = arena;
  parser.use_macros = use_macros;
  parser.ir = parser_parse_block(&parser, 0);

  Macros cached_macros;
  cached_macros.len = macros->len;
  cached_macros.cap = cached_macros.len;
  cached_macros.items = arena_alloc(arena, cached_macros.cap * sizeof(Macro));
  memcpy(cached_macros.items, macros->items, cached_macros.len * sizeof(Macro));

  FilePaths cached_included_files;
  cached_included_files.len = included_files->len;
  cached_included_files.cap = cached_included_files.len;
  cached_included_files.items = arena_alloc(arena, cached_included_files.cap * sizeof(Str));
  memcpy(cached_included_files.items, included_files->items,
         cached_included_files.len * sizeof(Str));

  CachedIr cached_ir = {
    file_path, parser.ir, cached_macros,
    cached_included_files, *arena,
  };
  DA_APPEND(cached_irs, cached_ir);

  free(parser.lexer.temp_sb.buffer);

  return parser.ir;
}

static void parser_parse_macro_def(Parser *parser, u32 row, u32 col) {
  Macro macro = {0};

  Token name_token = parser_expect_token(parser, MASK(TT_IDENT));
  macro.name = copy_str(name_token.lexeme, parser->arena);

  macro.row = row;
  macro.col = col;

  parser_expect_token(parser, MASK(TT_OBRACKET));

  Args arg_names = {0};

  Token next_token = parser_peek_token(parser);
  while (!next_token.eof && next_token.id != TT_CBRACKET) {
    Token arg_token = parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_UNPACK));
    if (arg_token.id == TT_UNPACK) {
      macro.has_unpack = true;

      arg_token = parser_expect_token(parser, MASK(TT_IDENT));
      Str arg_name = copy_str(arg_token.lexeme, parser->arena);
      DA_APPEND(arg_names, arg_name);

      break;
    }

    Str arg_name = copy_str(arg_token.lexeme, parser->arena);
    DA_APPEND(arg_names, arg_name);

    next_token = parser_peek_token(parser);
  }

  macro.arg_names.len = arg_names.len;
  macro.arg_names.items = arena_alloc(parser->arena, macro.arg_names.len * sizeof(Str));
  memcpy(macro.arg_names.items, arg_names.items, macro.arg_names.len * sizeof(Str));

  free(arg_names.items);

  parser_expect_token(parser, MASK(TT_CBRACKET));
  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));

  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(*parser->macros, macro);
}

static IrExpr *parser_parse_expr(Parser *parser, bool is_short);

static IrExprDict parser_parse_dict(Parser *parser) {
  IrExprDict dict = {0};

  Fields fields = {0};

  while (parser_peek_token(parser).id != TT_CCURLY) {
    IrExpr *key = parser_parse_expr(parser, false);
    parser_expect_token(parser, MASK(TT_COLON));
    IrExpr *expr = parser_parse_expr(parser, false);

    IrField field = { key, expr };
    DA_APPEND(fields, field);
  }

  dict.len = fields.len;
  dict.items = arena_alloc(parser->arena, dict.len * sizeof(IrField));
  memcpy(dict.items, fields.items, dict.len * sizeof(IrField));

  free(fields.items);

  parser_expect_token(parser, MASK(TT_CCURLY));

  return dict;
}

static IrExprLambda parser_parse_lambda(Parser *parser) {
  IrExprLambda lambda = {0};

  Args args = {0};

  Token arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                                MASK(TT_CBRACKET));
  while (arg_token.id != TT_CBRACKET) {
    Str arg = copy_str(arg_token.lexeme, parser->arena);
    DA_APPEND(args, arg);

    arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                            MASK(TT_CBRACKET));
  }

  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  lambda.args.len = args.len;
  lambda.args.items = arena_alloc(parser->arena, lambda.args.len * sizeof(Str));
  memcpy(lambda.args.items, args.items, args.len * sizeof(Str));

  free(args.items);

  Token token = parser_peek_token(parser);
  if (!token.eof && token.id == TT_IMPORT) {
    parser_next_token(parser);

    Token intrinsic_name_token = parser_expect_token(parser, MASK(TT_STR));
    Str intrinsic_name = {
      intrinsic_name_token.lexeme.ptr + 1,
      intrinsic_name_token.lexeme.len - 2,
    };
    lambda.intrinsic_name = copy_str(intrinsic_name, parser->arena);
  } else {
    lambda.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                             MASK(TT_CBRACKET) |
                                             MASK(TT_RHOMBUS));

    if (!parser_peek_token(parser).eof &&
        parser_peek_token(parser).id == TT_RHOMBUS)
      parser_next_token(parser);
  }

  return lambda;
}

static IrExprMatch parser_parse_match(Parser *parser) {
  IrExprMatch match = {0};

  match.src = parser_parse_expr(parser, false);

  Cases cases = {0};

  Token token;
  while (!(token = parser_peek_token(parser)).eof && token.id != TT_CPAREN) {
    IrExpr *pattern = parser_parse_expr(parser, false);
    parser_expect_token(parser, MASK(TT_RIGHT_ARROW));
    IrExpr *expr = parser_parse_expr(parser, false);

    IrCase _case = { pattern, expr };
    DA_APPEND(cases, _case);
  }

  match.cases.len = cases.len;
  match.cases.items = arena_alloc(parser->arena, match.cases.len * sizeof(IrCase));
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

static IrExpr *parser_parse_expr(Parser *parser, bool is_short) {
  IrExpr *expr = arena_alloc(parser->arena, sizeof(IrExpr));
  *expr = (IrExpr) {0};

  Token token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_STR) |
                                            MASK(TT_IDENT) | MASK(TT_INT) |
                                            MASK(TT_FLOAT) | MASK(TT_BOOL) |
                                            MASK(TT_OCURLY) | MASK(TT_OBRACKET) |
                                            MASK(TT_DOUBLE_ARROW));

  expr->meta.file_path = token.file_path;
  expr->meta.row = token.row;
  expr->meta.col = token.col;

  bool found_atom = true;

  switch (token.id) {
  case TT_STR: {
    expr->kind = IrExprKindString;

    expr->as.string = copy_str(STR(token.lexeme.ptr + 1, token.lexeme.len - 2),
                                   parser->arena);
  } break;

  case TT_IDENT: {
    expr->kind = IrExprKindIdent;
    expr->as.ident = copy_str(token.lexeme, parser->arena);
  } break;

  case TT_INT: {
    expr->kind = IrExprKindInt;
    expr->as._int = str_to_i64(token.lexeme);
  } break;

  case TT_FLOAT: {
    expr->kind = IrExprKindFloat;
    expr->as._float = str_to_f64(token.lexeme);
  } break;

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool = str_eq(token.lexeme, STR_LIT("true"));
  } break;

  case TT_OBRACKET: {
    Lexer prev_lexer = parser->lexer;

    IrBlock block = parser_parse_block(parser, MASK(TT_CBRACKET));

    parser_expect_token(parser, MASK(TT_CBRACKET));

    if (!parser_peek_token(parser).eof &&
        parser_peek_token(parser).id == TT_RIGHT_ARROW) {
      load_lexer(parser, &prev_lexer);

      expr->kind = IrExprKindLambda;
      expr->as.lambda = parser_parse_lambda(parser);
    } else {
      expr->kind = IrExprKindList;
      expr->as.list = block;
    }
  } break;

  case TT_OCURLY: {
    expr->kind = IrExprKindDict;
    expr->as.dict = parser_parse_dict(parser);
  } break;

  case TT_DOUBLE_ARROW: {
    expr->kind = IrExprKindSelf;
  } break;

  default: {
    found_atom = false;
  } break;
  }

  if (!found_atom) {
    token = parser_peek_token(parser);

    switch (token.id) {
    case TT_LET: {
      parser_next_token(parser);

      Token name_token = parser_expect_token(parser, MASK(TT_IDENT));

      expr->kind = IrExprKindVarDef;
      expr->as.var_def.name = copy_str(name_token.lexeme, parser->arena);
      expr->as.var_def.expr = parser_parse_expr(parser, false);

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_IF: {
      parser_next_token(parser);

      expr->kind = IrExprKindIf;
      expr->as._if.cond = parser_parse_expr(parser, false);

      expr->as._if.if_body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                                        MASK(TT_ELIF) |
                                                        MASK(TT_ELSE));

      Token next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                                     MASK(TT_ELIF) |
                                                     MASK(TT_ELSE));

      Elifs elifs = {0};

      while (next_token.id == TT_ELIF) {
        IrElif elif;
        elif.cond = parser_parse_expr(parser, false);
        elif.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                               MASK(TT_ELIF) |
                                               MASK(TT_ELSE));
        DA_APPEND(elifs, elif);

        next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                                 MASK(TT_ELIF) |
                                                 MASK(TT_ELSE));
      }

      expr->as._if.elifs.len = elifs.len;
      expr->as._if.elifs.items =
        arena_alloc(parser->arena, expr->as._if.elifs.len * sizeof(IrElif));
      memcpy(expr->as._if.elifs.items, elifs.items, expr->as._if.elifs.len * sizeof(IrElif));

      free(elifs.items);

      expr->as._if.has_else = next_token.id == TT_ELSE;
      if (expr->as._if.has_else) {
        expr->as._if.else_body = parser_parse_block(parser, MASK(TT_CPAREN));
        parser_expect_token(parser, MASK(TT_CPAREN));
      }
    } break;

    case TT_MACRO: {
      parser_next_token(parser);

      parser_parse_macro_def(parser, expr->meta.row, expr->meta.col);

      return expr;
    } break;

    case TT_WHILE: {
      parser_next_token(parser);

      expr->kind = IrExprKindWhile;
      expr->as._while.cond = parser_parse_expr(parser, false);
      expr->as._while.body = parser_parse_block(parser, MASK(TT_CPAREN));

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_USE: {
      parser_next_token(parser);

      Str module_path = parser_expect_token(parser, MASK(TT_STR)).lexeme;
      module_path.ptr += 1;
      module_path.len -= 2;

      parser_expect_token(parser, MASK(TT_CPAREN));

      Str prefix = get_file_dir(*parser->file_path);
      char *prefix_cstr = str_to_cstr(prefix);
      char *include_paths[INCLUDE_PATHS_LEN] = INCLUDE_PATHS;
      include_paths[0] = prefix_cstr;
      StringBuilder path_sb = {0};
      Str code = { NULL, (u32) -1 };
      Str *path = NULL;

      Arena arena = {0};

      for (u32 i = 0; i < ARRAY_LEN(include_paths); ++i) {
        sb_push(&path_sb, include_paths[i]);
        sb_push_str(&path_sb, module_path);

        if (parser->use_macros) {
          sb_push_str(&path_sb, STR_LIT(".abm\0"));

          code = read_file_arena(path_sb.buffer, &arena);
        }

        if (code.len != (u32) -1) {
          --path_sb.len; // exclude NULL-terminator
          path = arena_alloc(&arena, sizeof(Str));
          *path = copy_str(sb_to_str(path_sb), &arena);

          break;
        } else {
          if (parser->use_macros)
            path_sb.len -= 5;

          sb_push_str(&path_sb, STR_LIT(".ae\0"));

          code = read_file_arena(path_sb.buffer, &arena);

          if (code.len != (u32) -1) {
            --path_sb.len; // exclude NULL-terminator
            path = arena_alloc(&arena, sizeof(Str));
            *path = copy_str(sb_to_str(path_sb), &arena);

            break;
          }
        }

        path_sb.len = 0;
      }

      if (prefix_cstr)
        free(prefix_cstr);
      if (path_sb.buffer)
        free(path_sb.buffer);

      if (code.len == (u32) -1) {
        PERROR(STR_FMT":%u:%u: ", "Could not import `"STR_FMT"` module\n",
               STR_ARG(*token.file_path), token.row + 1,
               token.col + 1, STR_ARG(module_path));
        exit(1);
      }

      bool already_included = false;

      for (u32 i = 0; i < parser->included_files->len; ++i) {
        if (str_eq(*parser->included_files->items[i], *path)) {
          already_included = true;

          break;
        }
      }

      if (!already_included) {
        Str magic = {
          code.ptr,
          sizeof(u32),
        };

        if (str_eq(magic, STR_LIT("ABM\0"))) {
          Macros macros = deserialize_macros((u8 *) code.ptr, code.len, parser->arena);

          if (parser->macros->cap < parser->macros->len + macros.len) {
             parser->macros->cap = parser->macros->len + macros.len;

            if (parser->macros->len == 0)
              parser->macros->items = malloc(sizeof(Macro));
            else
              parser->macros->items = realloc(parser->macros->items,
                                              parser->macros->cap * sizeof(Macro));

            memcpy(parser->macros->items + parser->macros->len,
                   macros.items, macros.len * sizeof(Macro));

            parser->macros->len += macros.len;
          }
        } else {
          expr->as.block = parse_ex(code, path, parser->macros,
                                    parser->included_files, &arena,
                                    parser->use_macros);
        }
      }
    } break;

    case TT_SET: {
      parser_next_token(parser);

      Str dest = parser_expect_token(parser, MASK(TT_IDENT)).lexeme;

      token = parser_peek_token(parser);
      if (token.id == TT_QOLON) {
        parser_next_token(parser);

        expr->kind = IrExprKindSetAt;
        expr->as.set_at.dest = copy_str(dest, parser->arena);
        expr->as.set_at.key = parser_parse_expr(parser, false);
        expr->as.set_at.value = parser_parse_expr(parser, false);
      } else {
        expr->kind = IrExprKindSet;
        expr->as.set.dest = copy_str(dest, parser->arena);
        expr->as.set.src = parser_parse_expr(parser, false);
      }

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_RET: {
      parser_next_token(parser);

      expr->kind = IrExprKindRet;
      expr->as.ret.has_expr = parser_peek_token(parser).id != TT_CPAREN;

      if (expr->as.ret.has_expr)
        expr->as.ret.expr = parser_parse_expr(parser, false);

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_MATCH: {
      parser_next_token(parser);

      expr->kind = IrExprKindMatch;
      expr->as.match = parser_parse_match(parser);
    } break;

    case TT_DO: {
      parser_next_token(parser);

      expr->kind = IrExprKindBlock;
      expr->as.block = parser_parse_block(parser, MASK(TT_CPAREN));

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    default: {
      expr->kind = IrExprKindFuncCall;
      expr->as.func_call.func = parser_parse_expr(parser, false);
      expr->as.func_call.args = parser_parse_block(parser, MASK(TT_CPAREN));

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;
    }
  }

  if (!is_short) {
    while (!(token = parser_peek_token(parser)).eof &&
           token.id == TT_QOLON) {
      parser_next_token(parser);

      IrExpr *get_at = arena_alloc(parser->arena, sizeof(IrExpr));
      get_at->kind = IrExprKindGetAt;
      get_at->as.get_at.src = expr;
      get_at->as.get_at.key = parser_parse_expr(parser, true);

      get_at->meta.file_path = token.file_path;
      get_at->meta.row = token.row;
      get_at->meta.col = token.col;

      expr = get_at;
    }
  }

  return expr;
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask) {
  Block block = {0};

  Token token = parser_peek_token(parser);
  while (!token.eof && !(MASK(token.id) & end_id_mask)) {
    IrExpr *expr = parser_parse_expr(parser, false);
    block_append(&block, expr, parser->arena);

    token = parser_peek_token(parser);
  }

  return (IrBlock) {
    block.items,
    block.len,
  };
}

Ir parse(Str code, Str *file_path) {
  Macros macros = {0};
  FilePaths included_files = {0};
  Arena arena = {0};
  Ir ir = parse_ex(code, file_path, &macros, &included_files, &arena, false);
  expand_macros_block(&ir, &macros, NULL, NULL, false, &arena, file_path, 0, 0);

  if (macros.items)
    free(macros.items);
  if (included_files.items)
    free(included_files.items);

  return ir;
}
