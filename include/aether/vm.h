#ifndef AETHER_VM
#define AETHER_VM

#include "aether/parser.h"
#include "aether/bytecode.h"
#include "arena.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"
#include "shl/shl-log.h"

#define MAX_INTRINSIC_ARGS_COUNT  10
#define INTRINSICS_HASH_TABLE_CAP 60
#define LABELS_HASH_TABLE_CAP     60

#define EXECUTE_FUNC(vm, args, func, meta, value_expected)              \
  do {                                                                  \
    Value *result = execute_func(vm, args, func, meta, value_expected); \
    if (vm->state != ExecStateContinue)                                 \
      return result;                                                    \
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
    if (vm->state != ExecStatContinue)                       \
      return result;                                          \
  } while (0)

#define EXECUTE_FUNC_SET(vm, dest, args, func, meta, value_expected) \
  do {                                                               \
    dest = execute_func(vm, args, func, meta, value_expected);       \
    if (vm->state != ExecStateContinue)                              \
      return dest;                                                   \
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

typedef enum {
  ExecStateContinue = 0,
  ExecStateReturn,
  ExecStateExit,
  ExecStateBreak,
} ExecState;

typedef struct Vm Vm;

typedef Value *(*IntrinsicFunc)(Vm *vm, Value **args);

typedef struct Intrinsic Intrinsic;

struct Intrinsic {
  Str            name;
  bool           has_return_value;
  u32            args_count;
  ValueKind      arg_kinds[MAX_INTRINSIC_ARGS_COUNT];
  IntrinsicFunc  func;
  Intrinsic     *next;
};

typedef struct {
  Intrinsic *items[INTRINSICS_HASH_TABLE_CAP];
} Intrinsics;

typedef struct Label Label;

struct Label {
  Str    name;
  u32    instr_index;
  Label *next;
};

typedef struct {
  Label *items[LABELS_HASH_TABLE_CAP];
} LabelsTable;

typedef Da(LabelsTable) LabelsTables;

struct Vm {
  Ir           *ir;
  Values        stack;
  Intrinsics    intrinsics;
  LabelsTables  labels;
  StackFrame   *frames;
  StackFrame   *frames_end;
  StackFrame   *current_frame;
  ListNode     *args;
  ExecState     state;
  i64           exit_code;
  Str           current_file_path;
  FuncValue    *current_func;
  u32           current_func_index;
};

struct Env {
  Macros        macros;
  FilePaths     included_files;
  IncludePaths  include_paths;
  CachedASTs    cached_asts;
  Vm           *vm;
  u32           refs_count;
};

void   execute_func(Vm *vm, FuncValue *func, InstrMeta *meta, bool value_ignored);
Value *execute(Vm *vm, Ir *ir, bool value_expected);

Value *stack_last(Vm *vm);

void intrinsics_append(Intrinsics *a, Intrinsic *b, u32 b_len, Arena *arena);

Vm   vm_create(i32 argc, char **argv, Intrinsics *intrinsics);
void vm_init(Vm *vm, ListNode *args, Intrinsics *intrinsics);
void vm_stop(Vm *Vm);
void vm_destroy(Vm *vm);

void begin_frame(Vm *vm);
void end_frame(Vm *vm);

#endif // AETHER_VM
