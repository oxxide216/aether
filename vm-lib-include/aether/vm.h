#ifndef AETHER_VM
#define AETHER_VM

#include "aether/ir.h"
#include "aether/parser.h"
#include "arena.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define MAX_INTRINSIC_ARGS_COUNT 8

#define EXECUTE_FUNC(vm, args, func, meta, value_expected) \
  do {                                                     \
    execute_func(vm, args, func, meta, value_expected);    \
    if (vm->state != ExecStateContinue)                    \
      return NULL;                                         \
  } while (0)

#define EXECUTE_EXPR(vm, expr, value_expected) \
  do {                                         \
    execute_expr(vm, expr, value_expected);    \
    if (vm->state != ExecStateContinue)        \
      return NULL;                             \
  } while (0)

#define EXECUTE_BLOCK(vm, block, value_expected) \
  do {                                           \
    execute_block(vm, block, value_expected);    \
    if (vm->state != ExecStateContinue)          \
      return NULL;                               \
  } while (0)

#define EXECUTE_FUNC_SET(vm, dest, args, func, meta, value_expected) \
  do {                                                               \
    dest = execute_func(vm, args, func, meta, value_expected);       \
    if (vm->state != ExecStateContinue)                              \
      return NULL;                                                   \
  } while (0)

#define EXECUTE_EXPR_SET(vm, dest, expr, value_expected) \
  do {                                                   \
    dest = execute_expr(vm, expr, value_expected);       \
    if (vm->state != ExecStateContinue)                  \
      return NULL;                                       \
  } while (0)

#define EXECUTE_BLOCK_SET(vm, dest, block, value_expected) \
  do {                                                     \
    dest = execute_block(vm, block, value_expected);       \
    if (vm->state != ExecStateContinue)                    \
      return NULL;                                         \
  } while (0)

#define PANIC(arena, ...)     \
  do {                           \
    ERROR(__VA_ARGS__);          \
    return value_unit(arena); \
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

typedef struct ListNode   ListNode;
typedef struct NamedValue NamedValue;
typedef Da(NamedValue)    NamedValues;
typedef struct DictValue  DictValue;
typedef Da(DictValue)     Dict;

typedef struct {
  IrArgs      args;
  IrBlock     body;
  NamedValues catched_values;
  Str         intrinsic_name;
} Func;

typedef struct Vm Vm;

typedef struct Value Value;

typedef enum {
  VarKindLocal = 0,
  VarKindGlobal,
  VarKindCatched,
} VarKind;

typedef struct {
  Str      name;
  Value   *value;
  VarKind  kind;
} Var;

typedef Da(Var) Vars;

typedef enum {
  ExecStateContinue = 0,
  ExecStateReturn,
  ExecStateExit,
} ExecState;

typedef Value *(*IntrinsicFunc)(Vm *vm, Value **args);

typedef struct {
  Str           name;
  bool          has_return_value;
  u32           args_count;
  ValueKind     arg_kinds[MAX_INTRINSIC_ARGS_COUNT];
  IntrinsicFunc func;
} Intrinsic;

typedef Da(Intrinsic) Intrinsics;

struct Vm {
  Vars         global_vars;
  Vars         local_vars;
  Intrinsics   intrinsics;
  Arena        arena;
  i32          argc;
  char       **argv;
  ListNode    *args;
  ExecState    state;
  i64          exit_code;
  bool         is_inside_of_func;
  Func         current_func_value;
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

ListNode *list_clone(Arena *arena, ListNode *list);
Dict      dict_clone(Arena *arena, Dict *dict);

Value *value_unit(Arena *arena);
Value *value_list(Arena *arena, ListNode *list);
Value *value_string(Arena *arena, Str string);
Value *value_int(Arena *arena, i64 _int);
Value *value_float(Arena *arena, f64 _float);
Value *value_bool(Arena *arena, bool _bool);
Value *value_dict(Arena *arena, Dict dict);
Value *value_func(Arena *arena, Func func);
Value *value_env(Arena *arena, Vm vm);

Value *value_clone(Arena *arena, Value *value);
bool   value_eq(Value *a, Value *b);

Value *execute_func(Vm *vm, Value **args, Func *func,
                    IrMetaData *meta, bool value_expected);
Value *execute_expr(Vm *vm, IrExpr *expr, bool value_expected);
Value *execute_block(Vm *vm, IrBlock *block, bool value_expected);
u32    execute(Ir *ir, i32 argc, char **argv, Arena *arena,
               Intrinsics *intrinsics, Value **result_value);

Vm   vm_create(i32 argc, char **argv, Intrinsics *intrinsics);
void vm_destroy(Vm *vm);

#endif // AETHER_VM
