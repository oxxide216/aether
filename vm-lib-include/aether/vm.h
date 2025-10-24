#ifndef AETHER_VM
#define AETHER_VM

#include "aether/ir.h"
#include "aether/rc-arena.h"
#include "shl_defs.h"
#include "shl_str.h"
#include "shl_log.h"

#define MAX_INTRINSIC_ARGS_COUNT 8

#define EXECUTE_FUNC(vm, func, value_expected)                \
  do {                                                        \
    ExecState state = execute_func(vm, func, value_expected); \
    if (state != ExecStateContinue)                           \
      return state;                                           \
  } while (0)

#define EXECUTE_EXPR(vm, expr, value_expected)                \
  do {                                                        \
    ExecState state = execute_expr(vm, expr, value_expected); \
    if (state != ExecStateContinue)                           \
      return state;                                           \
  } while (0)

#define EXECUTE_BLOCK(vm, block, value_expected)                \
  do {                                                          \
    ExecState state = execute_block(vm, block, value_expected); \
    if (state != ExecStateContinue)                             \
      return state;                                             \
  } while (0)

#define PANIC(...)      \
  do {                  \
    ERROR(__VA_ARGS__); \
    return false;       \
  } while(0)

typedef enum {
  ValueKindUnit = 0,
  ValueKindList,
  ValueKindString,
  ValueKindInt,
  ValueKindFloat,
  ValueKindBool,
  ValueKindDict,
  ValueKindFunc,
} ValueKind;

typedef struct {
  Str  str;
  Str *begin;
} String;

typedef struct ListNode ListNode;

typedef struct DictValue DictValue;

typedef Da(DictValue) Dict;

typedef struct NamedValue  NamedValue;

typedef Da(NamedValue) NamedValues;

typedef struct {
  IrArgs      args;
  IrBlock     body;
  NamedValues catched_values;
  Str         intrinsic_name;
} Func;

typedef union {
  ListNode *list;
  String    string;
  i64       _int;
  f64       _float;
  bool      _bool;
  Dict      dict;
  Func      func;
} ValueAs;

typedef struct {
  ValueKind kind;
  ValueAs   as;
  u32       refs_count;
} Value;

struct ListNode {
  Value    *value;
  bool      is_static;
  ListNode *next;
};

struct DictValue {
  Value *key;
  Value *value;
};

struct NamedValue {
  Str    name;
  Value *value;
};

typedef enum {
  ExecStateContinue = 0,
  ExecStateReturn,
  ExecStateExit,
} ExecState;

typedef struct Vm Vm;

typedef Da(Value *) ValueStack;

typedef struct {
  Str    name;
  Value *value;
  bool   is_global;
} Var;

typedef Da(Var) Vars;

typedef bool (*IntrinsicFunc)(Vm *vm);

typedef struct {
  Str           name;
  bool          has_return_value;
  u32           args_count;
  ValueKind     arg_kinds[MAX_INTRINSIC_ARGS_COUNT];
  IntrinsicFunc func;
} Intrinsic;

typedef Da(Intrinsic) Intrinsics;

struct Vm {
  ValueStack  stack;
  Vars        global_vars;
  Vars        local_vars;
  Intrinsics  intrinsics;
  RcArena    *rc_arena;
  ListNode   *args;
  i64         exit_code;
  bool        is_inside_of_func;
  Func        current_func_value;
};

void      list_use(RcArena *rc_arena, ListNode *list);
ListNode *list_clone(RcArena *rc_arena, ListNode *list);
Dict      dict_clone(RcArena *rc_arena, Dict *dict);

void value_stack_push_unit(ValueStack *stack);
void value_stack_push_list(ValueStack *stack, RcArena *rc_arena, ListNode *list);
void value_stack_push_string(ValueStack *stack, RcArena *rc_arena, Str string);
void value_stack_push_int(ValueStack *stack, RcArena *rc_arena, i64 _int);
void value_stack_push_float(ValueStack *stack, RcArena *rc_arena, f64 _float);
void value_stack_push_bool(ValueStack *stack, RcArena *rc_arena, bool _bool);
void value_stack_push_dict(ValueStack *stack, RcArena *rc_arena, Dict dict);
void value_stack_push_func(ValueStack *stack, RcArena *rc_arena, Func func);

Value *value_stack_pop(ValueStack *stack);
Value *value_stack_get(ValueStack *stack, u32 index);

Value *value_clone(RcArena *rc_arena, Value *value);
void   value_free(Value *value, RcArena *rc_arena);

ExecState execute_func(Vm *vm, Func *func, bool value_expected);
ExecState execute_expr(Vm *vm, IrExpr *expr, bool value_expected);
ExecState execute_block(Vm *vm, IrBlock *block, bool value_expected);
u32       execute(Ir *ir, i32 argc, char **argv, RcArena *rc_arena,
                  Intrinsics *intrinsics, Value **result_value);

void cleanup(Vm *vm);

#endif // AETHER_VM
