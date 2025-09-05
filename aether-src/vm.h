#ifndef VM_H
#define VM_H

#include "air.h"

typedef enum {
  ValueKindUnit = 0,
  ValueKindList,
  ValueKindStrLit,
  ValueKindNumber,
} ValueKind;

typedef struct Value Value;

typedef Da(Value) List;
typedef union {
  List list;
  Str  str_lit;
  i64  number;
} ValueAs;

struct Value {
  ValueKind kind;
  ValueAs   as;
};

typedef struct Vm Vm;

typedef Value (*IntrinsicFunc)(Vm *vm, IrBlock *args);

typedef struct {
  Str           name;
  u32           args_count;
  IntrinsicFunc func;
} Intrinsic;

typedef IrExprFuncDef Func;

typedef Da(Func) Funcs;

typedef struct {
  Str   name;
  Value value;
} Var;

typedef Da(Var) Vars;

struct Vm {
  Funcs      funcs;
  Vars       vars;
};

Value execute_expr(Vm *vm, IrExpr *expr);
void  execute(Ir *ir);

#endif // VM_H
