#ifndef IR_H
#define IR_H

#include "shl/shl-defs.h"
#include "shl/shl-str.h"

typedef struct IrExpr IrExpr;

typedef struct {
  IrExpr **items;
  u32      len;
} IrBlock;

typedef enum {
  IrExprKindBlock = 0,
  IrExprKindFuncCall,
  IrExprKindVarDef,
  IrExprKindIf,
  IrExprKindWhile,
  IrExprKindSet,
  IrExprKindGetAt,
  IrExprKindSetAt,
  IrExprKindList,
  IrExprKindIdent,
  IrExprKindString,
  IrExprKindInt,
  IrExprKindFloat,
  IrExprKindBool,
  IrExprKindLambda,
  IrExprKindDict,
  IrExprKindRet,
  IrExprKindMatch,
  IrExprKindSelf,
  IrExprKindBreak,
} IrExprKind;

typedef struct {
  Str *items;
  u32  len;
} IrArgs;

typedef struct {
  IrExpr  *func;
  IrBlock  args;
} IrExprFuncCall;

typedef struct {
  Str     name;
  IrExpr *expr;
} IrExprVarDef;

typedef struct {
  IrExpr  *cond;
  IrBlock  body;
} IrElif;

typedef struct {
  IrElif *items;
  u32     len;
} IrElifs;

typedef struct {
  IrExpr  *cond;
  IrBlock  if_body;
  IrElifs  elifs;
  bool     has_else;
  IrBlock  else_body;
} IrExprIf;

typedef struct {
  IrExpr  *cond;
  IrBlock  body;
} IrExprWhile;

typedef struct {
  Str     dest;
  IrExpr *src;
} IrExprSet;

typedef struct {
  IrExpr *src;
  IrExpr *key;
} IrExprGetAt;

typedef struct {
  Str     dest;
  IrExpr *key;
  IrExpr *value;
} IrExprSetAt;

typedef struct {
  IrBlock content;
} IrExprList;

typedef struct {
  Str ident;
} IrExprIdent;

typedef struct {
  Str lit;
} IrExprString;

typedef struct {
  i64 _int;
} IrExprInt;

typedef struct {
  f64 _float;
} IrExprFloat;

typedef struct {
  bool _bool;
} IrExprBool;

typedef struct {
  IrArgs  args;
  IrBlock body;
  Str     intrinsic_name;
} IrExprLambda;

typedef struct {
  IrExpr *key;
  IrExpr *expr;
} IrField;

typedef struct {
  IrField *items;
  u32      len;
} IrFields;

typedef IrFields IrExprDict;

typedef struct {
  bool    has_expr;
  IrExpr *expr;
} IrExprRet;

typedef struct {
  IrExpr *pattern;
  IrExpr *expr;
} IrCase;

typedef struct {
  IrCase *items;
  u32     len;
} IrCases;

typedef struct {
  IrExpr  *src;
  IrCases  cases;
  IrExpr  *any;
} IrExprMatch;

typedef struct {
  IrExpr *expr;
} IrExprBreak;

#pragma pack(push, 1)
typedef union {
  IrBlock        block;
  IrExprFuncCall func_call;
  IrExprVarDef   var_def;
  IrExprIf       _if;
  IrExprWhile    _while;
  IrExprSet      set;
  IrExprGetAt    get_at;
  IrExprSetAt    set_at;
  IrBlock        list;
  Str            ident;
  Str            string;
  i64            _int;
  f64            _float;
  bool           _bool;
  IrExprLambda   lambda;
  IrExprDict     dict;
  IrExprRet      ret;
  IrExprMatch    match;
  IrExprBreak    _break;
} IrExprAs;
#pragma pack(pop)

typedef struct {
  Str *file_path;
  u16  row, col;
} IrExprMeta;

#pragma pack(push, 1)
struct IrExpr {
  u8         kind;
  u8         is_dead;
  u8         is_macro;
  IrExprAs   as;
  IrExprMeta meta;
};
#pragma pack(pop)

typedef IrBlock Ir;

#endif // IR_H
