#ifndef VM_H
#define VM_H

#include "air.h"

typedef enum {
  ValueKindUnit = 0,
  ValueKindList,
  ValueKindStrLit,
  ValueKindNumber,
  ValueKindBool,
} ValueKind;

typedef struct Value Value;

typedef Da(Value) List;

typedef union {
  List list;
  Str  str_lit;
  i64  number;
  bool _bool;
} ValueAs;

struct Value {
  ValueKind kind;
  ValueAs   as;
};

typedef struct Vm Vm;

typedef Value (*IntrinsicFunc)(Vm *vm, IrBlock *args, bool is_inside_of_func);

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

typedef IrArgs Args;

struct Vm {
  Funcs funcs;
  Vars  local_vars;
  Vars  global_vars;
  Args  args;
};

Value execute_expr(Vm *vm, IrExpr *expr, bool is_inside_of_func);
void  execute(Ir *ir, i32 argc, char **argv);

#endif // VM_H
