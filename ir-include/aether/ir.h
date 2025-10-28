#ifndef IR_H
#define IR_H

#include "shl/shl-defs.h"
#include "shl/shl-str.h"

typedef struct IrExpr IrExpr;

typedef Da(IrExpr *) IrBlock;

typedef enum {
  IrExprKindBlock = 0,
  IrExprKindFuncCall,
  IrExprKindVarDef,
  IrExprKindIf,
  IrExprKindWhile,
  IrExprKindSet,
  IrExprKindList,
  IrExprKindIdent,
  IrExprKindString,
  IrExprKindInt,
  IrExprKindFloat,
  IrExprKindBool,
  IrExprKindLambda,
  IrExprKindDict,
  IrExprKindRet,
  IrExprKindSelfCall,
} IrExprKind;

typedef Da(Str) IrArgs;

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

typedef Da(IrElif) IrElifs;

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
  IrExpr *dest;
  IrExpr *src;
} IrExprSet;

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

typedef Da(IrField) IrExprDict;

typedef struct {
  bool    has_expr;
  IrExpr *expr;
} IrExprRet;

typedef struct {
  IrBlock  args;
} IrExprSelfCall;

typedef Da(IrField) IrFields;

typedef union {
  IrBlock        block;
  IrExprFuncCall func_call;
  IrExprVarDef   var_def;
  IrExprIf       _if;
  IrExprWhile    _while;
  IrExprSet      set;
  IrExprList     list;
  IrExprIdent    ident;
  IrExprString   string;
  IrExprInt      _int;
  IrExprFloat    _float;
  IrExprBool     _bool;
  IrExprLambda   lambda;
  IrExprDict     dict;
  IrExprRet      ret;
  IrExprSelfCall self_call;
} IrExprAs;

struct IrExpr {
  IrExprKind kind;
  IrExprAs   as;
};

typedef IrBlock Ir;

#endif // IR_H
