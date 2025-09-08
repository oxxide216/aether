#ifndef VM_H
#define VM_H

#include "ir.h"
#include "rc-arena.h"

typedef enum {
  ValueKindUnit = 0,
  ValueKindList,
  ValueKindStr,
  ValueKindNumber,
  ValueKindBool,
} ValueKind;

typedef struct ListNode ListNode;

typedef union {
  ListNode *list;
  Str       str;
  i64       number;
  bool      _bool;
} ValueAs;

typedef struct {
  ValueKind kind;
  ValueAs   as;
} Value;

struct ListNode {
  Value     value;
  ListNode *next;
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

typedef IrArgs Args;

struct Vm {
  Funcs     funcs;
  Vars      local_vars;
  Vars      global_vars;
  ListNode *args;
  RcArena  *rc_arena;
  bool      is_inside_of_func;
};

Value execute_expr(Vm *vm, IrExpr *expr);
void  execute(Ir *ir, i32 argc, char **argv, RcArena *rc_arena);

#endif // VM_H
