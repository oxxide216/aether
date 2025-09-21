#ifndef IR_H
#define IR_H

#include "shl_defs.h"
#include "shl_str.h"

typedef struct IrExpr IrExpr;

typedef Da(IrExpr *) IrBlock;

typedef enum {
  IrExprKindBlock = 0,
  IrExprKindFuncDef,
  IrExprKindFuncCall,
  IrExprKindVarDef,
  IrExprKindIf,
  IrExprKindList,
  IrExprKindIdent,
  IrExprKindStrLit,
  IrExprKindNumber,
  IrExprKindBool,
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
  IrBlock  if_body;
  IrBlock  else_body;
  bool     has_else;
} IrExprIf;

typedef struct {
  IrBlock content;
} IrExprList;

typedef struct {
  Str ident;
} IrExprIdent;

typedef struct {
  Str lit;
} IrExprStrLit;

typedef struct {
  i64 number;
} IrExprNumber;

typedef struct {
  bool _bool;
} IrExprBool;

typedef union {
  IrBlock        block;
  IrExprFuncDef  func_def;
  IrExprFuncCall func_call;
  IrExprVarDef   var_def;
  IrExprIf       _if;
  IrExprList     list;
  IrExprIdent    ident;
  IrExprStrLit   str_lit;
  IrExprNumber   number;
  IrExprBool     _bool;
} IrExprAs;

struct IrExpr {
  IrExprKind kind;
  IrExprAs   as;
};

typedef IrBlock Ir;

#endif // IR_H
