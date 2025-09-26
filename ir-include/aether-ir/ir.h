#ifndef IR_H
#define IR_H

#include "../../ir-src/shl_defs.h"
#include "../../ir-src/shl_str.h"

typedef struct IrExpr IrExpr;

typedef Da(IrExpr *) IrBlock;

typedef enum {
  IrExprKindBlock = 0,
  IrExprKindFuncDef,
  IrExprKindFuncCall,
  IrExprKindVarDef,
  IrExprKindIf,
  IrExprKindWhile,
  IrExprKindSet,
  IrExprKindField,
  IrExprKindList,
  IrExprKindIdent,
  IrExprKindString,
  IrExprKindNumber,
  IrExprKindBool,
  IrExprKindLambda,
  IrExprKindRecord,
} IrExprKind;

typedef Da(Str) IrArgs;

typedef struct {
  Str     name;
  IrArgs  args;
  IrBlock body;
} IrExprFuncDef;

typedef struct {
  Str     name;
  IrBlock args;
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
  Str     dest;
  IrExpr *src;
} IrExprSet;

typedef struct {
  Str     record;
  Str     field;
  bool    is_set;
  IrExpr *expr;
} IrExprField;

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
  i64 number;
} IrExprNumber;

typedef struct {
  bool _bool;
} IrExprBool;

typedef struct {
  IrArgs  args;
  IrBlock body;
} IrExprLambda;

typedef struct {
  Str     name;
  IrExpr *expr;
} IrField;

typedef Da(IrField) IrExprRecord;

typedef union {
  IrBlock        block;
  IrExprFuncDef  func_def;
  IrExprFuncCall func_call;
  IrExprVarDef   var_def;
  IrExprIf       _if;
  IrExprWhile    _while;
  IrExprSet      set;
  IrExprField    field;
  IrExprList     list;
  IrExprIdent    ident;
  IrExprString   string;
  IrExprNumber   number;
  IrExprBool     _bool;
  IrExprLambda   lambda;
  IrExprRecord   record;
} IrExprAs;

struct IrExpr {
  IrExprKind kind;
  IrExprAs   as;
};

typedef IrBlock Ir;

#endif // IR_H
