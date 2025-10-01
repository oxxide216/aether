#ifndef VM_H
#define VM_H

#include "aether-ir/ir.h"
#include "aether-ir/rc-arena.h"
#include "../../vm-src/shl_log.h"

#define EXECUTE_FUNC(vm, name, args, value_expected)   \
  do {                                                 \
    if (!execute_func(vm, name, args, value_expected)) \
      return false;                                    \
  } while(0)

#define EXECUTE_EXPR(vm, expr, value_expected)   \
  do {                                           \
    if (!execute_expr(vm, expr, value_expected)) \
      return false;                              \
  } while(0)

#define EXECUTE_BLOCK(vm, block, value_expected)   \
  do {                                             \
    if (!execute_block(vm, block, value_expected)) \
      return false;                                \
  } while(0)

#define PANIC(...)      \
  do {                  \
    ERROR(__VA_ARGS__); \
    vm->exit_code = 1;  \
    return false;       \
  } while (0)

typedef struct NamedValue  NamedValue;

typedef Da(NamedValue) NamedValues;

typedef struct {
  IrExprFuncDef def;
  NamedValues   catched_values;
} Func;

typedef Da(Func) Funcs;

typedef enum {
  ValueKindUnit = 0,
  ValueKindList,
  ValueKindString,
  ValueKindInt,
  ValueKindFloat,
  ValueKindBool,
  ValueKindFunc,
  ValueKindRecord,
} ValueKind;

typedef struct ListNode ListNode;

typedef struct {
  Str         name;
  IrArgs      args;
  IrBlock     body;
  NamedValues catched_values;
} ValueFunc;

typedef NamedValues Record;

typedef union {
  ListNode  *list;
  Str        string;
  i64        _int;
  f64        _float;
  bool       _bool;
  ValueFunc  func;
  Record     record;
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

struct NamedValue {
  Str   name;
  Value value;
};

typedef Da(Value) ValueStack;

typedef struct Vm Vm;

typedef bool (*IntrinsicFunc)(Vm *vm);

typedef struct {
  Str           name;
  u32           args_count;
  bool          has_return_value;
  IntrinsicFunc func;
} Intrinsic;

typedef Da(Intrinsic) Intrinsics;

typedef IrArgs Args;

typedef struct {
  Str   name;
  Value value;
} Var;

typedef Da(Var) Vars;

struct Vm {
  ValueStack  stack;
  Funcs       funcs;
  Vars        local_vars;
  Vars        global_vars;
  ListNode   *args;
  RcArena    *rc_arena;
  Intrinsics  intrinsics;
  bool        is_inside_of_func;
  u32         exit_code;
};

void      list_use(RcArena *rc_arena, ListNode *list);
ListNode *list_clone(RcArena *rc_arena, ListNode *list);

void value_stack_push_unit(ValueStack *stack);
void value_stack_push_list(ValueStack *stack, ListNode *nodes);
void value_stack_push_string(ValueStack *stack, Str string);
void value_stack_push_int(ValueStack *stack, i64 _int);
void value_stack_push_float(ValueStack *stack, f64 _float);
void value_stack_push_bool(ValueStack *stack, bool _bool);
void value_stack_push_record(ValueStack *stack, Record record);

Value  value_stack_pop(ValueStack *stack);
Value *value_stack_get(ValueStack *stack, u32 index);

void free_value(Value *value, RcArena *rc_arena);

bool execute_func(Vm *vm, Str name, u32 args_len, bool value_expected);
bool execute_expr(Vm *vm, IrExpr *expr, bool value_expected);
bool execute_block(Vm *vm, IrBlock *block, bool value_expected);
u32  execute(Ir *ir, i32 argc, char **argv,
             RcArena *rc_arena, Intrinsics *intrinsics);

#endif // VM_H
