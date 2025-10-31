#ifndef AETHER_VM
#define AETHER_VM

#include "aether/ir.h"
#include "aether/rc-arena.h"
#include "aether/parser.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define MAX_INTRINSIC_ARGS_COUNT 8

#define EXECUTE_FUNC(vm, func, meta, value_expected)                \
  do {                                                              \
    ExecState state = execute_func(vm, func, meta, value_expected); \
    if (state != ExecStateContinue)                                 \
      return state;                                                 \
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
  ValueKindEnv,
} ValueKind;

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

typedef struct Vm Vm;

typedef struct Value Value;

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
  RcArena     rc_arena;
  ListNode   *args;
  i64         exit_code;
  bool        is_inside_of_func;
  Func        current_func_value;
};

typedef struct {
  Macros    macros;
  FilePaths included_files;
  Vm        vm;
} Env;

typedef union {
  ListNode *list;
  Str       string;
  i64       _int;
  f64       _float;
  bool      _bool;
  Dict      dict;
  Func      func;
  Env      *env;
} ValueAs;

struct Value {
  ValueKind kind;
  ValueAs   as;
  u32       refs_count;
};

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

void      list_use(RcArena *rc_arena, ListNode *list);
ListNode *list_clone(RcArena *rc_arena, ListNode *list);
Dict      dict_clone(RcArena *rc_arena, Dict *dict);

void value_stack_push_unit(ValueStack *stack, RcArena *rc_arena);
void value_stack_push_list(ValueStack *stack, RcArena *rc_arena, ListNode *list);
void value_stack_push_string(ValueStack *stack, RcArena *rc_arena, Str string);
void value_stack_push_int(ValueStack *stack, RcArena *rc_arena, i64 _int);
void value_stack_push_float(ValueStack *stack, RcArena *rc_arena, f64 _float);
void value_stack_push_bool(ValueStack *stack, RcArena *rc_arena, bool _bool);
void value_stack_push_dict(ValueStack *stack, RcArena *rc_arena, Dict dict);
void value_stack_push_func(ValueStack *stack, RcArena *rc_arena, Func func);
void value_stack_push_env(ValueStack *stack, RcArena *rc_arena, Vm vm);

Value *value_stack_pop(ValueStack *stack);
Value *value_stack_get(ValueStack *stack, u32 index);

Value *value_clone(RcArena *rc_arena, Value *value);
void   value_free(Value *value, RcArena *rc_arena, bool free_ptr);
bool   value_eq(Value *a, Value *b);

ExecState execute_func(Vm *vm, Func *func, IrMetaData *meta, bool value_expected);
ExecState execute_expr(Vm *vm, IrExpr *expr, bool value_expected);
ExecState execute_block(Vm *vm, IrBlock *block, bool value_expected);
u32       execute(Ir *ir, i32 argc, char **argv, RcArena *rc_arena,
                  Intrinsics *intrinsics, Value **result_value);

Vm   vm_create(i32 argc, char **argv, Intrinsics *intrinsics);
void vm_destroy(Vm *vm);

#endif // AETHER_VM
