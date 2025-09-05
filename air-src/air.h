#ifndef AIR_H
#define AIR_H

#include "shl_defs.h"
#include "shl_str.h"

typedef struct IrExpr IrExpr;

typedef Da(IrExpr *) IrBlock;

typedef enum {
  IrExprKindFuncDef = 0,
  IrExprKindFuncCall,
  IrExprKindList,
  IrExprKindIdent,
  IrExprKindStrLit,
  IrExprKindNumber,
  IrExprKindVarDef,
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
  Str     name;
  IrExpr *expr;
} IrExprVarDef;

typedef union {
  IrExprFuncDef  func_def;
  IrExprFuncCall func_call;
  IrExprList     list;
  IrExprIdent    ident;
  IrExprStrLit   str_lit;
  IrExprNumber   number;
  IrExprVarDef   var_def;
} IrExprAs;

struct IrExpr {
  IrExprKind kind;
  IrExprAs   as;
};

typedef IrBlock Ir;

#endif // AIR_H
