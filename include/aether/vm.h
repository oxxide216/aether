#ifndef AETHER_VM
#define AETHER_VM

#include "aether/ir.h"
#include "aether/parser.h"
#include "arena.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define MAX_INTRINSIC_ARGS_COUNT 10

#define EXECUTE_FUNC(vm, args, func, meta, is_moved, value_expected)             \
  do {                                                                           \
    Value *result = execute_func(vm, args, func, is_moved meta, value_expected); \
    if (vm->state == ExecStateReturn)                                            \
      vm->state = ExecStateContinue;                                             \
    if (vm->state != ExecStateContinue)                                          \
      return result;                                                             \
  } while (0)

#define EXECUTE_EXPR(vm, expr, value_expected)              \
  do {                                                      \
    Value *result = execute_expr(vm, expr, value_expected); \
    if (vm->state != ExecStateContinue)                     \
      return result;                                        \
  } while (0)

#define EXECUTE_BLOCK(vm, block, value_expected)              \
  do {                                                        \
    Value *result = execute_block(vm, block, value_expected); \
    if (vm->state != ExecStateContinue)                       \
      return result;                                          \
  } while (0)

#define EXECUTE_FUNC_SET(vm, dest, args, func, is_moved, meta, value_expected) \
  do {                                                                         \
    dest = execute_func(vm, args, func, is_moved, meta, value_expected);       \
    if (vm->state != ExecStateContinue)                                        \
      return dest;                                                             \
  } while (0)

#define EXECUTE_EXPR_SET(vm, dest, expr, value_expected) \
  do {                                                   \
    dest = execute_expr(vm, expr, value_expected);       \
    if (vm->state != ExecStateContinue)                  \
      return dest;                                       \
  } while (0)

#define EXECUTE_BLOCK_SET(vm, dest, block, value_expected) \
  do {                                                     \
    dest = execute_block(vm, block, value_expected);       \
    if (vm->state != ExecStateContinue)                    \
      return dest;                                         \
  } while (0)

#define PANIC(frame, ...)     \
  do {                        \
    ERROR(__VA_ARGS__);       \
    return value_unit(frame); \
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
  ValueKindBytes,
} ValueKind;

typedef struct ListNode   ListNode;
typedef struct NamedValue NamedValue;
typedef Da(NamedValue)    NamedValues;
typedef struct DictValue  DictValue;
typedef Da(DictValue)     Dict;

typedef struct {
  Str  str;
} String;

typedef struct Vm Vm;
typedef struct Value Value;
typedef Da(Value *) Values;

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
typedef Da(Var *) VarRefs;

typedef struct StackFrame StackFrame;

struct StackFrame {
  Values      values;
  Arena       arena;
  Vars        vars;
  bool        can_lookup_through;
  StackFrame *next;
  StackFrame *prev;
};

typedef struct Func Func;

struct Func {
  IrArgs       args;
  IrBlock      body;
  NamedValues  catched_values_names;
  Func        *parent_func;
  Str          intrinsic_name;
  u32          refs_count;
};

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
  Vars        global_vars;
  Intrinsics  intrinsics;
  StackFrame *frames;
  StackFrame *frames_end;
  StackFrame *current_frame;
  ListNode   *args;
  ExecState   state;
  i64         exit_code;
  Func       *current_func;
  Str         current_file_path;
};

typedef struct {
  Macros    macros;
  FilePaths included_files;
  Vm        vm;
  u32       refs_count;
} Env;

typedef struct {
  u8 *ptr;
  u32 len;
} Bytes;

typedef union {
  ListNode *list;
  String    string;
  i64      _int;
  f64       _float;
  bool      _bool;
  Dict      dict;
  Func     *func;
  Env      *env;
  Bytes     bytes;
} ValueAs;

struct Value {
  ValueKind   kind;
  ValueAs     as;
  StackFrame *frame;
  u32         refs_count;
  bool        is_atom;
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

ListNode *list_clone(ListNode *list, StackFrame *frame);
Dict      dict_clone(Dict *dict, StackFrame *frame);

Value *value_unit(StackFrame *frame);
Value *value_list(ListNode *nodes, StackFrame *frame);
Value *value_string(Str string, StackFrame *frame);
Value *value_int(i64 _int, StackFrame *frame);
Value *value_float(f64 _float, StackFrame *frame);
Value *value_bool(bool _bool, StackFrame *frame);
Value *value_dict(Dict dict, StackFrame *frame);
Value *value_func(Func func, StackFrame *frame);
Value *value_env(Vm vm, StackFrame *frame);
Value *value_bytes(Bytes bytes, StackFrame *frame);

Value *value_alloc(StackFrame *frame);
Value *value_clone(Value *value, StackFrame *frame);
void   value_free(Value *value);
bool   value_eq(Value *a, Value *b);

Value *execute_func(Vm *vm, Value **args, Func *func,
                    IrExprMeta *meta, bool value_expected);
Value *execute_expr(Vm *vm, IrExpr *expr, bool value_expected);
Value *execute_block(Vm *vm, IrBlock *block, bool value_expected);
u32    execute(Ir *ir, i32 argc, char **argv, Arena *arena,
               Intrinsics *intrinsics, Value **result_value);

Vm   vm_create(i32 argc, char **argv, Intrinsics *intrinsics);
void vm_init(Vm *vm, ListNode *args, Intrinsics *intrinsics);
void vm_stop(Vm *Vm);
void vm_destroy(Vm *vm);

void begin_frame(Vm *vm);
void end_frame(Vm *vm);

#endif // AETHER_VM
