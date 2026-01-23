#include <wchar.h>

#define SHL_DEFS_LL_ALLOC(size) arena_alloc(arena, size)

#include "aether/parser.h"
#include "aether/common.h"
#include "aether/macros.h"
#include "aether/deserializer.h"
#include "aether/io.h"
#include "lexgen/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "aether/grammar.h"
#include "shl/shl-log.h"

#define MASK(id) (1lu << (id))

typedef enum {
  TokenStatusOk = 0,
  TokenStatusEmpty,
  TokenStatusEOF,
} TokenStatus;

typedef struct {
  u64   id;
  Str   lexeme;
  u16   row, col;
} Token;

typedef Da(Token) Tokens;

typedef struct {
  Str              code;
  u32              row, col;
  TransitionTable *table;
  StringBuilder    temp_sb;
} Lexer;

typedef struct {
  Tokens        tokens;
  Macros       *macros;
  Str          *file_path;
  FilePaths    *included_files;
  IncludePaths *include_paths;
  CachedASTs   *cached_asts;
  Arena        *arena;
  bool          use_macros;
  u32           index;
  u32           current_func;
  Da(u32)       label_indices;
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
  "set",
  "use",
  "ret",
  "import",
  "match",
  "do",
  "while",
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
  "`=>`",
  "`\\`",
  "<->"
  "int",
  "float",
  "bool",
  "identifier",
};

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

    *token = (Token) { id, lexeme, row, col };

    return TokenStatusOk;
  }

  return TokenStatusEOF;
}

static Token *parser_next_token(Parser *parser) {
  if (parser->index == parser->tokens.len)
    return NULL;
  return parser->tokens.items + parser->index++;
}

static Token *parser_peek_token(Parser *parser) {
  if (parser->index == parser->tokens.len)
    return NULL;
  return parser->tokens.items + parser->index;
}

static void print_id_mask(u64 id_mask) {
  u32 ids_count = 0;
  for (u64 i = 0; i < ARRAY_LEN(token_names); ++i)
    if (MASK(i) & id_mask)
      ++ids_count;

  for (u64 i = 0, j = 0; i < 64 && j < ids_count; ++i) {
    if ((1lu << i) & id_mask) {
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
    PERROR(STR_FMT": ", "Expected ", STR_ARG(*parser->file_path));
    print_id_mask(id_mask);
    fprintf(stderr, ", but got EOF\n");
    exit(1);
  }

  if (MASK(token->id) & id_mask)
    return token;

  PERROR(STR_FMT":%u:%u: ", "Expected ",
         STR_ARG(*parser->file_path),
         token->row + 1, token->col + 1);
  print_id_mask(id_mask);
  fprintf(stderr, ", but got `"STR_FMT"`\n",
          STR_ARG(token->lexeme));
  exit(1);
}

static void include_file(FilePaths *included_files, Str *new_file) {
  for (u32 i = 0; i < included_files->len; ++i)
    if (str_eq(*included_files->items[i], *new_file))
      return;

  DA_APPEND(*included_files, new_file);
}

static Expr  *parser_parse_expr(Parser *parser, bool is_short);
static Exprs  parser_parse_block(Parser *parser, u64 end_id_mask);

Exprs parse_ex(Str code, Str *file_path, Macros *macros,
               FilePaths *included_files, IncludePaths *include_paths,
               CachedASTs *cached_asts, Arena *arena, bool use_macros,
               u32 *root_label_index) {
  u64 code_hash = str_hash(code);

  for (u32 i = 0; i < cached_asts->len; ++i) {
    CachedAST *cached_ast = cached_asts->items + i;

    if (cached_ast->code_hash == code_hash) {
      DA_EXTEND(*macros, cached_ast->macros);
      DA_EXTEND(*included_files, cached_ast->included_files);

      *arena = cached_ast->arena;

      return cached_ast->ast;
    }
  }

  include_file(included_files, file_path);

  Parser parser = {0};

  Lexer lexer = {0};
  lexer.code = code;
  lexer.table = get_transition_table();

  TokenStatus status = TokenStatusEmpty;
  Token token;
  while (status != TokenStatusEOF) {
    status = lex(&lexer, &token, file_path, arena);
    if (status != TokenStatusEOF && status != TokenStatusEmpty)
      DA_APPEND(parser.tokens, token);
  }

  free(lexer.temp_sb.buffer);

  parser.macros = macros;
  parser.file_path = file_path;
  parser.included_files = included_files;
  parser.cached_asts = cached_asts;
  parser.include_paths = include_paths;
  parser.arena = arena;
  parser.use_macros = use_macros;

  if (root_label_index)
    DA_APPEND(parser.label_indices, *root_label_index);
  else
    DA_APPEND(parser.label_indices, 0);

  Exprs ast = parser_parse_block(&parser, 0);

  if (root_label_index)
    *root_label_index = parser.label_indices.items[0];

  if (parser.label_indices.items)
    free(parser.label_indices.items);

  if (parser.tokens.items)
    free(parser.tokens.items);

  Macros cached_macros;
  cached_macros.len = macros->len;
  cached_macros.cap = cached_macros.len;
  cached_macros.items = arena_alloc(arena, cached_macros.cap * sizeof(Macro));
  memcpy(cached_macros.items, macros->items, cached_macros.len * sizeof(Macro));

  FilePaths cached_included_files;
  cached_included_files.len = included_files->len;
  cached_included_files.cap = cached_included_files.len;
  cached_included_files.items = arena_alloc(arena, cached_included_files.cap * sizeof(Str *));
  memcpy(cached_included_files.items, included_files->items,
         cached_included_files.len * sizeof(Str *));

  CachedAST cached_ast = {
    code_hash, ast, cached_macros,
    cached_included_files, *arena,
  };
  DA_APPEND(*cached_asts, cached_ast);

  return ast;
}

static void parser_parse_macro_def(Parser *parser) {
  Macro macro = {0};

  Token *begin_token = parser_next_token(parser);
  macro.row = begin_token->row;
  macro.col = begin_token->col;

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  macro.name = copy_str(name_token->lexeme, parser->arena);

  parser_expect_token(parser, MASK(TT_BACKSLASH));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_RIGHT_ARROW) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_UNPACK));
    if (arg_token->id == TT_UNPACK) {
      macro.has_unpack = true;

      arg_token = parser_expect_token(parser, MASK(TT_IDENT));
      Str arg_name = copy_str(arg_token->lexeme, parser->arena);
      DA_APPEND(macro.arg_names, arg_name);

      break;
    }

    Str arg_name = copy_str(arg_token->lexeme, parser->arena);
    DA_APPEND(macro.arg_names, arg_name);

    next_token = parser_peek_token(parser);
  }

  Str *new_items = arena_alloc(parser->arena, macro.arg_names.len * sizeof(Str));
  memcpy(new_items, macro.arg_names.items, macro.arg_names.len * sizeof(Str));
  free(macro.arg_names.items);
  macro.arg_names.items = new_items;

  parser_expect_token(parser, MASK(TT_RIGHT_ARROW));

  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(*parser->macros, macro);
}

static ExprDict parser_parse_dict(Parser *parser) {
  ExprDict result = {0};

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CCURLY) {
    Expr *expr = parser_parse_expr(parser, false);
    DA_APPEND(result.content, expr);

    parser_expect_token(parser, MASK(TT_COLON));

    expr = parser_parse_expr(parser, false);
    DA_APPEND(result.content, expr);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_CCURLY));

  return result;
}

static ExprFunc parser_parse_lambda(Parser *parser) {
  ExprFunc result = {0};

  Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                                 MASK(TT_RIGHT_ARROW));
  while (arg_token && arg_token->id != TT_RIGHT_ARROW) {
    Str arg = copy_str(arg_token->lexeme, parser->arena);
    DA_APPEND(result.args, arg);

    arg_token = parser_expect_token(parser, MASK(TT_IDENT) |
                                            MASK(TT_RIGHT_ARROW));
  }

  Str *new_items = arena_alloc(parser->arena, result.args.len * sizeof(Str));
  memcpy(new_items, result.args.items, result.args.len * sizeof(Str));
  free(result.args.items);
  result.args.items = new_items;

  Token *token = parser_peek_token(parser);
  if (token && token->id == TT_IMPORT) {
    parser_next_token(parser);

    Token *intrinsic_name_token = parser_expect_token(parser, MASK(TT_STR));
    Str intrinsic_name = {
      intrinsic_name_token->lexeme.ptr + 1,
      intrinsic_name_token->lexeme.len - 2,
    };
    result.intrinsic_name = copy_str(intrinsic_name, parser->arena);
  } else {
    u32 prev_func = parser->current_func;

    result.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                             MASK(TT_CBRACKET) |
                                             MASK(TT_CCURLY) |
                                             MASK(TT_RHOMBUS));

    parser->current_func = prev_func;

    Token *token = parser_peek_token(parser);
    if (token && token->id == TT_RHOMBUS)
      parser_next_token(parser);
  }

  return result;
}

static ExprMatch parser_parse_match(Parser *parser) {
  parser_next_token(parser);

  ExprMatch result = {0};
  result.value = parser_parse_expr(parser, false);

  Token *token;
  while ((token = parser_peek_token(parser)) && token->id != TT_CPAREN) {
    if (token->id == TT_UNPACK) {
      parser_next_token(parser);
      parser_expect_token(parser, MASK(TT_EQ_ARROW));

      result.else_branch = parser_parse_expr(parser, false);

      break;
    }

    Expr *value = parser_parse_expr(parser, false);
    DA_APPEND(result.values, value);

    parser_expect_token(parser, MASK(TT_EQ_ARROW));

    Expr *branch = parser_parse_expr(parser, false);
    DA_APPEND(result.branches, branch);
  }

  parser_expect_token(parser, MASK(TT_CPAREN));

  return result;
}

static ExprIf parser_parse_if(Parser *parser) {
  parser_next_token(parser);

  ExprIf result = {0};
  result.cond = parser_parse_expr(parser, false);
  result.if_body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                              MASK(TT_ELIF) |
                                              MASK(TT_ELSE));

  ExprIf *last = &result;

  Token *next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                                  MASK(TT_ELIF) |
                                                  MASK(TT_ELSE));

  while (next_token && next_token->id == TT_ELIF) {
    Expr *elif = arena_alloc(parser->arena, sizeof(Expr));
    elif->kind = ExprKindIf;
    elif->as._if.cond = parser_parse_expr(parser, false);
    elif->as._if.if_body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                               MASK(TT_ELIF) |
                                               MASK(TT_ELSE));
    elif->meta.file_path = parser->file_path;
    elif->meta.row = next_token->row;
    elif->meta.col = next_token->col;

    DA_APPEND(last->else_body, elif);
    last = &elif->as._if;

    next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                             MASK(TT_ELIF) |
                                             MASK(TT_ELSE));
  }

  if (next_token->id == TT_ELSE) {
    last->else_body = parser_parse_block(parser, MASK(TT_CPAREN));

    parser_expect_token(parser, MASK(TT_CPAREN));
  }

  return result;
}

static ExprWhile parser_parse_while(Parser *parser) {
  parser_next_token(parser);

  ExprWhile result = {0};
  result.cond = parser_parse_expr(parser, false);
  result.body = parser_parse_block(parser, MASK(TT_CPAREN));

  parser_expect_token(parser, MASK(TT_CPAREN));

  return result;
}

static Expr *parser_parse_expr(Parser *parser, bool is_short) {
  Expr *expr = arena_alloc(parser->arena, sizeof(Expr));

  Token *first_token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_STR) |
                                                   MASK(TT_IDENT) | MASK(TT_INT) |
                                                   MASK(TT_FLOAT) | MASK(TT_BOOL) |
                                                   MASK(TT_OCURLY) | MASK(TT_OBRACKET) |
                                                   MASK(TT_BACKSLASH) | MASK(TT_DOUBLE_ARROW));

  expr->meta.file_path = parser->file_path;
  expr->meta.row = first_token->row;
  expr->meta.col = first_token->col;

  bool found_atom = true;

  switch (first_token->id) {
  case TT_STR: {
    expr->as.primitive.value = arena_alloc(parser->arena, sizeof(Value));
    expr->as.primitive.value->kind = ValueKindString;
    expr->as.primitive.value->refs_count = (u16) -1;
    expr->as.primitive.value->as.string.str = copy_str(STR(first_token->lexeme.ptr + 1,
                                                           first_token->lexeme.len - 2),
                                                       parser->arena);
  } break;

  case TT_IDENT: {
    expr->kind = ExprKindIdent;
    expr->as.ident.name = copy_str(first_token->lexeme, parser->arena);
  } break;

  case TT_INT: {
    expr->kind = ExprKindPrimitive;
    expr->as.primitive.value = arena_alloc(parser->arena, sizeof(Value));
    expr->as.primitive.value->kind = ValueKindInt;
    expr->as.primitive.value->refs_count = (u16) -1;
    expr->as.primitive.value->as._int = str_to_i64(first_token->lexeme);

  } break;

  case TT_FLOAT: {
    expr->kind = ExprKindPrimitive;
    expr->as.primitive.value = arena_alloc(parser->arena, sizeof(Value));
    expr->as.primitive.value->kind = ValueKindFloat;
    expr->as.primitive.value->refs_count = (u16) -1;
    expr->as.primitive.value->as._float = str_to_f64(first_token->lexeme);
  } break;

  case TT_BOOL: {
    expr->kind = ExprKindPrimitive;
    expr->as.primitive.value = arena_alloc(parser->arena, sizeof(Value));
    expr->as.primitive.value->kind = ValueKindBool;
    expr->as.primitive.value->refs_count = (u16) -1;
    expr->as.primitive.value->as._bool = str_eq(first_token->lexeme, STR_LIT("true"));
  } break;

  case TT_OBRACKET: {
    expr->kind = ExprKindList;
    expr->as.list.content = parser_parse_block(parser, MASK(TT_CBRACKET));

    parser_expect_token(parser, MASK(TT_CBRACKET));
  } break;

  case TT_BACKSLASH: {
    expr->kind = ExprKindFunc;
    expr->as.func = parser_parse_lambda(parser);
  } break;

  case TT_OCURLY: {
    expr->kind = ExprKindDict;
    expr->as.dict = parser_parse_dict(parser);
  } break;

  case TT_DOUBLE_ARROW: {
    expr->kind = ExprKindSelf;
  } break;

  default: {
    found_atom = false;
  } break;
  }

  if (!found_atom) {
    Token *token = parser_peek_token(parser);

    switch (token->id) {
    case TT_LET: {
      parser_next_token(parser);

      Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));

      expr->kind = ExprKindLet;
      expr->as.let.name = name_token->lexeme;
      expr->as.let.value = parser_parse_expr(parser, false);

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_IF: {
      expr->kind = ExprKindIf;
      expr->as._if = parser_parse_if(parser);
    } break;

    case TT_MACRO: {
      parser_parse_macro_def(parser);

      expr->kind = ExprKindBlock;
    } break;

    case TT_USE: {
      parser_next_token(parser);

      Str module_path = parser_expect_token(parser, MASK(TT_STR))->lexeme;
      module_path.ptr += 1;
      module_path.len -= 2;

      parser_expect_token(parser, MASK(TT_CPAREN));

      StringBuilder path_sb = {0};
      Str code = { NULL, (u32) -1 };
      Str path = {0};

      for (u32 i = 0; i < parser->include_paths->len; ++i) {
        sb_push_str(&path_sb, parser->include_paths->items[i]);
        sb_push_str(&path_sb, module_path);

        if (parser->use_macros) {
          sb_push_str(&path_sb, STR_LIT(".abm\0"));

          code = read_file_arena(path_sb.buffer, parser->arena);
        }

        if (code.len != (u32) -1) {
          --path_sb.len; // exclude NULL-terminator
          path = copy_str(sb_to_str(path_sb), parser->arena);

          break;
        } else {
          if (parser->use_macros)
            path_sb.len -= 5;

          sb_push_str(&path_sb, STR_LIT(".ae\0"));

          code = read_file_arena(path_sb.buffer, parser->arena);

          if (code.len != (u32) -1) {
            --path_sb.len; // exclude NULL-terminator
            path = copy_str(sb_to_str(path_sb), parser->arena);

            break;
          }
        }

        path_sb.len = 0;
      }

      if (path_sb.buffer)
        free(path_sb.buffer);

      if (code.len == (u32) -1) {
        PERROR(STR_FMT":%u:%u: ", "Could not import `"STR_FMT"` module\n",
               STR_ARG(*parser->file_path), token->row + 1,
               token->col + 1, STR_ARG(module_path));
        exit(1);
      }

      for (u32 i = 0; i < parser->included_files->len; ++i) {
        if (str_eq(*parser->included_files->items[i], path)) {
          expr->kind = ExprKindBlock;
          return expr;
        }
      }

      Str dir = get_file_dir(path);
      DA_APPEND(*parser->include_paths, dir);

      Str magic = {
        code.ptr,
        sizeof(u32),
      };

      Str *path_ptr = arena_alloc(parser->arena, sizeof(Str));
      *path_ptr = path;

      include_file(parser->included_files, path_ptr);

      if (str_eq(magic, STR_LIT("ABM\0"))) {
        Macros macros = deserialize_macros((u8 *) code.ptr, code.len,
                                           parser->included_files,
                                           parser->arena);

        if (parser->macros->cap < parser->macros->len + macros.len) {
           parser->macros->cap = parser->macros->len + macros.len;

          if (parser->macros->len == 0)
            parser->macros->items = malloc(parser->macros->cap * sizeof(Macro));
          else
            parser->macros->items = realloc(parser->macros->items,
                                            parser->macros->cap * sizeof(Macro));
        }

        memcpy(parser->macros->items + parser->macros->len,
               macros.items, macros.len * sizeof(Macro));

        parser->macros->len += macros.len;
      } else {
        Arena arena = {0};
        expr->kind = ExprKindBlock;
        expr->as.block = parse_ex(code, path_ptr, parser->macros,
                                  parser->included_files, parser->include_paths,
                                  parser->cached_asts, &arena, parser->use_macros,
                                  &parser->label_indices.items[0]);
      }
    } break;

    case TT_SET: {
      parser_next_token(parser);

      expr->kind = ExprKindSet;

      Expr *temp_expr = parser_parse_expr(parser, true);
      DA_APPEND(expr->as.set.chain, temp_expr);

      token = parser_peek_token(parser);

      while (token && token->id == TT_QOLON) {
        parser_next_token(parser);

        temp_expr = parser_parse_expr(parser, false);
        DA_APPEND(expr->as.set.chain, temp_expr);

        token = parser_peek_token(parser);
      }

      expr->as.set.new = parser_parse_expr(parser, false);

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_RET: {
      parser_next_token(parser);

      expr->kind = ExprKindRet;

      token = parser_peek_token(parser);
      if (token && token->id != TT_CPAREN)
        expr->as.ret.value = parser_parse_expr(parser, false);

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_MATCH: {
      expr->kind = ExprKindMatch;
      expr->as.match = parser_parse_match(parser);
    } break;

    case TT_DO: {
      parser_next_token(parser);

      expr->kind = ExprKindBlock;
      expr->as.block = parser_parse_block(parser, MASK(TT_CPAREN));

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;

    case TT_WHILE: {
      expr->kind = ExprKindWhile;
      expr->as._while = parser_parse_while(parser);
    } break;

    default: {
      expr->kind = ExprKindFuncCall;
      expr->as.func_call.func = parser_parse_expr(parser, false);

      Token *token = parser_peek_token(parser);
      while (token && token->id != TT_CPAREN) {
        Expr *temp_expr = parser_parse_expr(parser, false);
        DA_APPEND(expr->as.func_call.args, temp_expr);

        token = parser_peek_token(parser);
      }

      parser_expect_token(parser, MASK(TT_CPAREN));
    } break;
    }
  }

  if (!is_short) {
    Token *begin_token = parser_peek_token(parser);
    Token *token = begin_token;
    if (token && token->id == TT_QOLON) {
      ExprGet get = {0};
      DA_APPEND(get.chain, expr);

      while (token && token->id == TT_QOLON) {
        parser_next_token(parser);

        Expr *expr = parser_parse_expr(parser, true);
        DA_APPEND(get.chain, expr);

        token = parser_peek_token(parser);
      }

      expr = arena_alloc(parser->arena, sizeof(Expr));
      expr->kind = ExprKindGet;
      expr->as.get = get;
      expr->meta.file_path = parser->file_path;
      expr->meta.row = begin_token->row;
      expr->meta.col = begin_token->col;
    }
  }

  return expr;
}

static Exprs parser_parse_block(Parser *parser, u64 end_id_mask) {
  Exprs result = {0};

  Token *token = parser_peek_token(parser);
  while (token && !(MASK(token->id) & end_id_mask)) {
    Expr *expr = parser_parse_expr(parser, false);
    DA_APPEND(result, expr);

    token = parser_peek_token(parser);
  }

  return result;
}

Ir parse(Str code, Str *file_path, CachedASTs *cached_asts) {
  Macros macros = {0};
  FilePaths included_files = {0};
  IncludePaths include_paths = {0};
  Arena arena = {0};

  Str file_dir = get_file_dir(*file_path);

  DA_APPEND(include_paths, file_dir);
  DA_APPEND(include_paths, STR_LIT("ae-src/"));
  DA_APPEND(include_paths, STR_LIT("/usr/include/aether/"));

  Exprs ast = parse_ex(code, file_path, &macros,
                       &included_files, &include_paths,
                       cached_asts, &arena, false, NULL);

  expand_macros_block(&ast, &macros, NULL, NULL, false,
                      &arena, file_path, 0, 0, false);

  Ir ir = ast_to_ir(&ast, &arena);

  if (macros.items)
    free(macros.items);

  if (included_files.items)
    free(included_files.items);

  if (include_paths.items)
    free(include_paths.items);

  return ir;
}
