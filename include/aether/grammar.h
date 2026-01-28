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
#define TT_SET 8
#define TT_USE 9
#define TT_RET 10
#define TT_IMPORT 11
#define TT_MATCH 12
#define TT_DO 13
#define TT_SET_EXCL 14
#define TT_OPAREN 15
#define TT_CPAREN 16
#define TT_OBRACKET 17
#define TT_CBRACKET 18
#define TT_OCURLY 19
#define TT_CCURLY 20
#define TT_STR 21
#define TT_UNPACK 22
#define TT_RIGHT_ARROW 23
#define TT_COLON 24
#define TT_QOLON 25
#define TT_RHOMBUS 26
#define TT_EQ_ARROW 27
#define TT_BACKSLASH 28
#define TT_DOUBLE_ARROW 29
#define TT_INT 30
#define TT_FLOAT 31
#define TT_IDENT 32

#define TTS_COLNT U33

TransitionTable *get_transition_table(void);

#ifdef LEXGEN_TRANSITION_TABLE_IMPLEMENTATION

TransitionCol table_col_whitespace[] = {
  { 1, 32, 32, 1 },
  { 1, 9, 9, 1 },
  { 1, 13, 13, 1 },
  { 1, -1, -1, 0 },
};

TransitionCol table_col_newline[] = {
  { 1, 10, 10, 0 },
};

TransitionCol table_col_comment[] = {
  { 1, 59, 59, 0 },
};

TransitionCol table_col_let[] = {
  { 1, 108, 108, 2 },
  { 2, 101, 101, 3 },
  { 3, 116, 116, 0 },
};

TransitionCol table_col_if[] = {
  { 1, 105, 105, 2 },
  { 2, 102, 102, 0 },
};

TransitionCol table_col_elif[] = {
  { 1, 101, 101, 2 },
  { 2, 108, 108, 3 },
  { 3, 105, 105, 4 },
  { 4, 102, 102, 0 },
};

TransitionCol table_col_else[] = {
  { 1, 101, 101, 2 },
  { 2, 108, 108, 3 },
  { 3, 115, 115, 4 },
  { 4, 101, 101, 0 },
};

TransitionCol table_col_macro[] = {
  { 1, 109, 109, 2 },
  { 2, 97, 97, 3 },
  { 3, 99, 99, 4 },
  { 4, 114, 114, 5 },
  { 5, 111, 111, 0 },
};

TransitionCol table_col_set[] = {
  { 1, 115, 115, 2 },
  { 2, 101, 101, 3 },
  { 3, 116, 116, 0 },
};

TransitionCol table_col_use[] = {
  { 1, 117, 117, 2 },
  { 2, 115, 115, 3 },
  { 3, 101, 101, 0 },
};

TransitionCol table_col_ret[] = {
  { 1, 114, 114, 2 },
  { 2, 101, 101, 3 },
  { 3, 116, 116, 0 },
};

TransitionCol table_col_import[] = {
  { 1, 105, 105, 2 },
  { 2, 109, 109, 3 },
  { 3, 112, 112, 4 },
  { 4, 111, 111, 5 },
  { 5, 114, 114, 6 },
  { 6, 116, 116, 0 },
};

TransitionCol table_col_match[] = {
  { 1, 109, 109, 2 },
  { 2, 97, 97, 3 },
  { 3, 116, 116, 4 },
  { 4, 99, 99, 5 },
  { 5, 104, 104, 0 },
};

TransitionCol table_col_do[] = {
  { 1, 100, 100, 2 },
  { 2, 111, 111, 0 },
};

TransitionCol table_col_set_excl[] = {
  { 1, 115, 115, 2 },
  { 2, 101, 101, 3 },
  { 3, 116, 116, 4 },
  { 4, 33, 33, 0 },
};

TransitionCol table_col_oparen[] = {
  { 1, 40, 40, 0 },
};

TransitionCol table_col_cparen[] = {
  { 1, 41, 41, 0 },
};

TransitionCol table_col_obracket[] = {
  { 1, 91, 91, 0 },
};

TransitionCol table_col_cbracket[] = {
  { 1, 93, 93, 0 },
};

TransitionCol table_col_ocurly[] = {
  { 1, 123, 123, 0 },
};

TransitionCol table_col_ccurly[] = {
  { 1, 125, 125, 0 },
};

TransitionCol table_col_str[] = {
  { 1, 34, 34, 0 },
  { 1, 39, 39, 0 },
};

TransitionCol table_col_unpack[] = {
  { 1, 46, 46, 2 },
  { 2, 46, 46, 3 },
  { 3, 46, 46, 0 },
};

TransitionCol table_col_right_arrow[] = {
  { 1, 45, 45, 2 },
  { 2, 62, 62, 0 },
};

TransitionCol table_col_colon[] = {
  { 1, 58, 58, 0 },
};

TransitionCol table_col_qolon[] = {
  { 1, 58, 58, 2 },
  { 2, 58, 58, 0 },
};

TransitionCol table_col_rhombus[] = {
  { 1, 60, 60, 2 },
  { 2, 62, 62, 0 },
};

TransitionCol table_col_eq_arrow[] = {
  { 1, 61, 61, 2 },
  { 2, 62, 62, 0 },
};

TransitionCol table_col_backslash[] = {
  { 1, 92, 92, 0 },
};

TransitionCol table_col_double_arrow[] = {
  { 1, 60, 60, 2 },
  { 2, 45, 45, 3 },
  { 3, 62, 62, 0 },
};

TransitionCol table_col_int[] = {
  { 1, 45, 45, 2 },
  { 1, -1, -1, 2 },
  { 2, 48, 57, 3 },
  { 3, 48, 57, 3 },
  { 3, -1, -1, 0 },
};

TransitionCol table_col_float[] = {
  { 1, 45, 45, 2 },
  { 1, -1, -1, 2 },
  { 2, 48, 57, 3 },
  { 3, 48, 57, 3 },
  { 3, -1, -1, 5 },
  { 5, 46, 46, 6 },
  { 6, 48, 57, 7 },
  { 7, 48, 57, 7 },
  { 7, -1, -1, 0 },
};

TransitionCol table_col_ident[] = {
  { 1, 97, 122, 2 },
  { 1, 65, 90, 2 },
  { 1, 95, 95, 2 },
  { 1, 45, 45, 2 },
  { 1, 33, 33, 2 },
  { 1, 63, 63, 2 },
  { 1, 35, 35, 2 },
  { 1, 36, 36, 2 },
  { 1, 37, 37, 2 },
  { 1, 94, 94, 2 },
  { 1, 38, 38, 2 },
  { 1, 42, 42, 2 },
  { 1, 43, 43, 2 },
  { 1, 47, 47, 2 },
  { 1, 61, 61, 2 },
  { 1, 60, 60, 2 },
  { 1, 62, 62, 2 },
  { 1, 124, 124, 2 },
  { 2, 97, 122, 2 },
  { 2, 65, 90, 2 },
  { 2, 95, 95, 2 },
  { 2, 45, 45, 2 },
  { 2, 33, 33, 2 },
  { 2, 63, 63, 2 },
  { 2, 35, 35, 2 },
  { 2, 36, 36, 2 },
  { 2, 37, 37, 2 },
  { 2, 94, 94, 2 },
  { 2, 38, 38, 2 },
  { 2, 42, 42, 2 },
  { 2, 43, 43, 2 },
  { 2, 47, 47, 2 },
  { 2, 61, 61, 2 },
  { 2, 60, 60, 2 },
  { 2, 62, 62, 2 },
  { 2, 124, 124, 2 },
  { 2, 48, 57, 2 },
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
  { table_col_set, sizeof(table_col_set) / sizeof(TransitionCol) },
  { table_col_use, sizeof(table_col_use) / sizeof(TransitionCol) },
  { table_col_ret, sizeof(table_col_ret) / sizeof(TransitionCol) },
  { table_col_import, sizeof(table_col_import) / sizeof(TransitionCol) },
  { table_col_match, sizeof(table_col_match) / sizeof(TransitionCol) },
  { table_col_do, sizeof(table_col_do) / sizeof(TransitionCol) },
  { table_col_set_excl, sizeof(table_col_set_excl) / sizeof(TransitionCol) },
  { table_col_oparen, sizeof(table_col_oparen) / sizeof(TransitionCol) },
  { table_col_cparen, sizeof(table_col_cparen) / sizeof(TransitionCol) },
  { table_col_obracket, sizeof(table_col_obracket) / sizeof(TransitionCol) },
  { table_col_cbracket, sizeof(table_col_cbracket) / sizeof(TransitionCol) },
  { table_col_ocurly, sizeof(table_col_ocurly) / sizeof(TransitionCol) },
  { table_col_ccurly, sizeof(table_col_ccurly) / sizeof(TransitionCol) },
  { table_col_str, sizeof(table_col_str) / sizeof(TransitionCol) },
  { table_col_unpack, sizeof(table_col_unpack) / sizeof(TransitionCol) },
  { table_col_right_arrow, sizeof(table_col_right_arrow) / sizeof(TransitionCol) },
  { table_col_colon, sizeof(table_col_colon) / sizeof(TransitionCol) },
  { table_col_qolon, sizeof(table_col_qolon) / sizeof(TransitionCol) },
  { table_col_rhombus, sizeof(table_col_rhombus) / sizeof(TransitionCol) },
  { table_col_eq_arrow, sizeof(table_col_eq_arrow) / sizeof(TransitionCol) },
  { table_col_backslash, sizeof(table_col_backslash) / sizeof(TransitionCol) },
  { table_col_double_arrow, sizeof(table_col_double_arrow) / sizeof(TransitionCol) },
  { table_col_int, sizeof(table_col_int) / sizeof(TransitionCol) },
  { table_col_float, sizeof(table_col_float) / sizeof(TransitionCol) },
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
