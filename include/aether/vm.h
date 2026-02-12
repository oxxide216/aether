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
    if (vm->state != ExecStatContinue)                        \
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

struct Vm {
  Ir            ir;
  Values        stack;
  Intrinsics    intrinsics;
  StackFrame   *frames;
  StackFrame   *frames_end;
  StackFrame   *current_frame;
  StackFrame   *temp_frame;
  u32           frame_begin;
  ListNode     *args;
  ExecState     state;
  i64           exit_code;
  Str           current_file_path;
  FuncValue    *current_func;
  LabelsTable  *current_func_labels;
  u16           recursion_level;
  u16           trace_level;
  u16           max_trace_level;
  bool          catch_kill_signal;
#ifdef EMSCRIPTEN
  u16           pending_fetches;
#endif
};

struct Env {
  Macros        macros;
  FilePaths     included_files;
  IncludePaths  include_paths;
  CachedASTs    cached_asts;
  CachedIrs     cached_irs;
  Vm           *vm;
  u32           refs_count;
};

void   execute_func(Vm *vm, FuncValue *func, InstrMeta *meta);
void   execute(Vm *vm, Instrs *instrs);
Value *execute_get(Vm *vm, Func *func);

Value *stack_last(Vm *vm);

void intrinsics_append(Intrinsics *a, Intrinsic *b, u32 b_len, Arena *arena);

Vm   vm_create(i32 argc, char **argv, Intrinsics *intrinsics);
void vm_init(Vm *vm, ListNode *args, Intrinsics *intrinsics);
void vm_stop(Vm *Vm);
void vm_destroy(Vm *vm);

void begin_frame(Vm *vm);
void reset_frame(StackFrame *frame);
void end_frame(Vm *vm);

#endif // AETHER_VM
