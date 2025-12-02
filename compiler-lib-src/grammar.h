#ifndef LEXGEN_TRANSITION_TABLE
#define LEXGEN_TRANSITION_TABLE

#define TT_WHITESPACE 0
#define TT_NEWLINE 1
#define TT_COMMENT 2
#define TT_LET 3
#define TT_IF 4
#define TT_ELIF 5
#define TT_ELSE 6
#define TT_MACRO 7
#define TT_WHILE 8
#define TT_SET 9
#define TT_USE 10
#define TT_RET 11
#define TT_IMPORT 12
#define TT_MATCH 13
#define TT_DO 14
#define TT_OPAREN 15
#define TT_CPAREN 16
#define TT_OBRACKET 17
#define TT_CBRACKET 18
#define TT_OCURLY 19
#define TT_CCURLY 20
#define TT_STR 21
#define TT_UNPACK 22
#define TT_RIGHT_ARROW 23
#define TT_RHOMBUS 24
#define TT_COLON 25
#define TT_QOLON 26
#define TT_DOUBLE_ARROW 27
#define TT_INT 28
#define TT_FLOAT 29
#define TT_BOOL 30
#define TT_IDENT 31

#define TTS_COUNT 32

TransitionTable *get_transition_table(void);

#ifdef LEXGEN_TRANSITION_TABLE_IMPLEMENTATION

TransitionCol table_col_whitespace[] = {
  { 1, ' ', ' ', 1 },
  { 1, '\t', '\t', 1 },
  { 1, '\r', '\r', 1 },
  { 1, -1, -1, 0 },
};

TransitionCol table_col_newline[] = {
  { 1, '\n', '\n', 0 },
};

TransitionCol table_col_comment[] = {
  { 1, ';', ';', 0 },
};

TransitionCol table_col_let[] = {
  { 1, 'l', 'l', 2 },
  { 2, 'e', 'e', 3 },
  { 3, 't', 't', 0 },
};

TransitionCol table_col_if[] = {
  { 1, 'i', 'i', 2 },
  { 2, 'f', 'f', 0 },
};

TransitionCol table_col_elif[] = {
  { 1, 'e', 'e', 2 },
  { 2, 'l', 'l', 3 },
  { 3, 'i', 'i', 4 },
  { 4, 'f', 'f', 0 },
};

TransitionCol table_col_else[] = {
  { 1, 'e', 'e', 2 },
  { 2, 'l', 'l', 3 },
  { 3, 's', 's', 4 },
  { 4, 'e', 'e', 0 },
};

TransitionCol table_col_macro[] = {
  { 1, 'm', 'm', 2 },
  { 2, 'a', 'a', 3 },
  { 3, 'c', 'c', 4 },
  { 4, 'r', 'r', 5 },
  { 5, 'o', 'o', 0 },
};

TransitionCol table_col_while[] = {
  { 1, 'w', 'w', 2 },
  { 2, 'h', 'h', 3 },
  { 3, 'i', 'i', 4 },
  { 4, 'l', 'l', 5 },
  { 5, 'e', 'e', 0 },
};

TransitionCol table_col_set[] = {
  { 1, 's', 's', 2 },
  { 2, 'e', 'e', 3 },
  { 3, 't', 't', 0 },
};

TransitionCol table_col_use[] = {
  { 1, 'u', 'u', 2 },
  { 2, 's', 's', 3 },
  { 3, 'e', 'e', 0 },
};

TransitionCol table_col_ret[] = {
  { 1, 'r', 'r', 2 },
  { 2, 'e', 'e', 3 },
  { 3, 't', 't', 0 },
};

TransitionCol table_col_import[] = {
  { 1, 'i', 'i', 2 },
  { 2, 'm', 'm', 3 },
  { 3, 'p', 'p', 4 },
  { 4, 'o', 'o', 5 },
  { 5, 'r', 'r', 6 },
  { 6, 't', 't', 0 },
};

TransitionCol table_col_match[] = {
  { 1, 'm', 'm', 2 },
  { 2, 'a', 'a', 3 },
  { 3, 't', 't', 4 },
  { 4, 'c', 'c', 5 },
  { 5, 'h', 'h', 0 },
};

TransitionCol table_col_do[] = {
  { 1, 'd', 'd', 2 },
  { 2, 'o', 'o', 0 },
};

TransitionCol table_col_oparen[] = {
  { 1, '(', '(', 0 },
};

TransitionCol table_col_cparen[] = {
  { 1, ')', ')', 0 },
};

TransitionCol table_col_obracket[] = {
  { 1, '[', '[', 0 },
};

TransitionCol table_col_cbracket[] = {
  { 1, ']', ']', 0 },
};

TransitionCol table_col_ocurly[] = {
  { 1, '{', '{', 0 },
};

TransitionCol table_col_ccurly[] = {
  { 1, '}', '}', 0 },
};

TransitionCol table_col_str[] = {
  { 1, '"', '"', 0 },
  { 1, '\'', '\'', 0 },
};

TransitionCol table_col_unpack[] = {
  { 1, '.', '.', 2 },
  { 2, '.', '.', 3 },
  { 3, '.', '.', 0 },
};

TransitionCol table_col_right_arrow[] = {
  { 1, '-', '-', 2 },
  { 2, '>', '>', 0 },
};

TransitionCol table_col_rhombus[] = {
  { 1, '<', '<', 2 },
  { 2, '>', '>', 0 },
};

TransitionCol table_col_colon[] = {
  { 1, ':', ':', 0 },
};

TransitionCol table_col_qolon[] = {
  { 1, ':', ':', 2 },
  { 2, ':', ':', 0 },
};

TransitionCol table_col_double_arrow[] = {
  { 1, '<', '<', 2 },
  { 2, '-', '-', 3 },
  { 3, '>', '>', 0 },
};

TransitionCol table_col_int[] = {
  { 1, '-', '-', 2 },
  { 1, -1, -1, 2 },
  { 2, '0', '9', 3 },
  { 3, '0', '9', 3 },
  { 3, -1, -1, 0 },
};

TransitionCol table_col_float[] = {
  { 1, '-', '-', 2 },
  { 1, -1, -1, 2 },
  { 2, '0', '9', 3 },
  { 3, '0', '9', 3 },
  { 3, -1, -1, 5 },
  { 5, '.', '.', 6 },
  { 6, '0', '9', 7 },
  { 7, '0', '9', 7 },
  { 7, -1, -1, 0 },
};

TransitionCol table_col_bool[] = {
  { 1, 't', 't', 2 },
  { 2, 'r', 'r', 3 },
  { 3, 'u', 'u', 4 },
  { 4, 'e', 'e', 0 },
  { 1, 'f', 'f', 2 },
  { 2, 'a', 'a', 3 },
  { 3, 'l', 'l', 4 },
  { 4, 's', 's', 5 },
  { 5, 'e', 'e', 0 },
};

TransitionCol table_col_ident[] = {
  { 1, 'a', 'z', 2 },
  { 1, 'A', 'Z', 2 },
  { 1, '_', '_', 2 },
  { 1, '-', '-', 2 },
  { 1, '!', '!', 2 },
  { 1, '?', '?', 2 },
  { 1, '#', '#', 2 },
  { 1, '$', '$', 2 },
  { 1, '%', '%', 2 },
  { 1, '^', '^', 2 },
  { 1, '&', '&', 2 },
  { 1, '*', '*', 2 },
  { 1, '+', '+', 2 },
  { 1, '/', '/', 2 },
  { 1, '=', '=', 2 },
  { 1, '<', '<', 2 },
  { 1, '>', '>', 2 },
  { 1, '|', '|', 2 },
  { 2, 'a', 'z', 2 },
  { 2, 'A', 'Z', 2 },
  { 2, '_', '_', 2 },
  { 2, '-', '-', 2 },
  { 2, '!', '!', 2 },
  { 2, '?', '?', 2 },
  { 2, '#', '#', 2 },
  { 2, '$', '$', 2 },
  { 2, '%', '%', 2 },
  { 2, '^', '^', 2 },
  { 2, '&', '&', 2 },
  { 2, '*', '*', 2 },
  { 2, '+', '+', 2 },
  { 2, '/', '/', 2 },
  { 2, '=', '=', 2 },
  { 2, '<', '<', 2 },
  { 2, '>', '>', 2 },
  { 2, '|', '|', 2 },
  { 2, '0', '9', 2 },
  { 2, -1, -1, 0 },
};

TransitionRow table_rows[] = {
  { table_col_whitespace, sizeof(table_col_whitespace) / sizeof(TransitionCol) },
  { table_col_newline, sizeof(table_col_newline) / sizeof(TransitionCol) },
  { table_col_comment, sizeof(table_col_comment) / sizeof(TransitionCol) },
  { table_col_let, sizeof(table_col_let) / sizeof(TransitionCol) },
  { table_col_if, sizeof(table_col_if) / sizeof(TransitionCol) },
  { table_col_elif, sizeof(table_col_elif) / sizeof(TransitionCol) },
  { table_col_else, sizeof(table_col_else) / sizeof(TransitionCol) },
  { table_col_macro, sizeof(table_col_macro) / sizeof(TransitionCol) },
  { table_col_while, sizeof(table_col_while) / sizeof(TransitionCol) },
  { table_col_set, sizeof(table_col_set) / sizeof(TransitionCol) },
  { table_col_use, sizeof(table_col_use) / sizeof(TransitionCol) },
  { table_col_ret, sizeof(table_col_ret) / sizeof(TransitionCol) },
  { table_col_import, sizeof(table_col_import) / sizeof(TransitionCol) },
  { table_col_match, sizeof(table_col_match) / sizeof(TransitionCol) },
  { table_col_do, sizeof(table_col_do) / sizeof(TransitionCol) },
  { table_col_oparen, sizeof(table_col_oparen) / sizeof(TransitionCol) },
  { table_col_cparen, sizeof(table_col_cparen) / sizeof(TransitionCol) },
  { table_col_obracket, sizeof(table_col_obracket) / sizeof(TransitionCol) },
  { table_col_cbracket, sizeof(table_col_cbracket) / sizeof(TransitionCol) },
  { table_col_ocurly, sizeof(table_col_ocurly) / sizeof(TransitionCol) },
  { table_col_ccurly, sizeof(table_col_ccurly) / sizeof(TransitionCol) },
  { table_col_str, sizeof(table_col_str) / sizeof(TransitionCol) },
  { table_col_unpack, sizeof(table_col_unpack) / sizeof(TransitionCol) },
  { table_col_right_arrow, sizeof(table_col_right_arrow) / sizeof(TransitionCol) },
  { table_col_rhombus, sizeof(table_col_rhombus) / sizeof(TransitionCol) },
  { table_col_colon, sizeof(table_col_colon) / sizeof(TransitionCol) },
  { table_col_qolon, sizeof(table_col_qolon) / sizeof(TransitionCol) },
  { table_col_double_arrow, sizeof(table_col_double_arrow) / sizeof(TransitionCol) },
  { table_col_int, sizeof(table_col_int) / sizeof(TransitionCol) },
  { table_col_float, sizeof(table_col_float) / sizeof(TransitionCol) },
  { table_col_bool, sizeof(table_col_bool) / sizeof(TransitionCol) },
  { table_col_ident, sizeof(table_col_ident) / sizeof(TransitionCol) },
};

TransitionTable table = {
  table_rows,
  sizeof(table_rows) / sizeof(TransitionRow),
};

TransitionTable *get_transition_table(void) {
  return &table;
};

#endif // LEXGEN_TRANSITION_TABLE_IMPLEMENTATION

#endif // LEXGEN_TRANSITION_TABLE
