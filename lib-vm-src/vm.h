#ifndef VM_H
#define VM_H

#include "ir.h"
#include "rc-arena.h"

typedef IrExprFuncDef Func;

typedef Da(Func) Funcs;

typedef enum {
  ValueKindUnit = 0,
  ValueKindList,
  ValueKindStr,
  ValueKindNumber,
  ValueKindBool,
  ValueKindFunc,
} ValueKind;

typedef struct ListNode ListNode;

typedef struct {
  Str     name;
  IrArgs  args;
  IrBlock body;
} ValueFunc;

typedef union {
  ListNode  *list;
  Str        str;
  i64        number;
  bool       _bool;
  ValueFunc  func;
} ValueAs;

typedef struct {
  ValueKind kind;
  ValueAs   as;
} Value;

struct ListNode {
  Value     value;
  bool      is_static;
  ListNode *next;
};

typedef Da(Value) ValueStack;

typedef struct Vm Vm;

typedef void (*IntrinsicFunc)(Vm *vm);

typedef struct {
  Str           name;
  u32           args_count;
  bool          has_return_value;
  IntrinsicFunc func;
} Intrinsic;

typedef Da(Intrinsic) Intrinsics;

typedef struct {
  Str name;
  u32 value_index;
} Var;

typedef Da(Var) Vars;

typedef IrArgs Args;

struct Vm {
  ValueStack  stack;
  Funcs       funcs;
  Vars        local_vars;
  Vars        global_vars;
  ListNode   *args;
  RcArena    *rc_arena;
  Intrinsics  intrinsics;
  bool        is_inside_of_func;
};

void value_stack_push_unit(ValueStack *stack);
void value_stack_push_list(ValueStack *stack, ListNode *nodes);
void value_stack_push_str(ValueStack *stack, Str str);
void value_stack_push_number(ValueStack *stack, i64 number);
void value_stack_push_bool(ValueStack *stack, bool _bool);

Value  value_stack_pop(ValueStack *stack);
Value *value_stack_get(ValueStack *stack, u32 index);

void execute_func(Vm *vm, Str name, ValueStack *args, bool value_expected);
void execute_expr(Vm *vm, IrExpr *expr, bool value_expected);
void execute(Ir *ir, i32 argc, char **argv,
             RcArena *rc_arena, Intrinsics *intrinsics);

#endif // VM_H
