#ifndef BYTECODE_VM
#define BYTECODE_VM

#include "aether/common.h"
#include "aether/arena.h"
#include "shl/shl-defs.h"
#include "shl/shl-str.h"

#define DICT_HASH_TABLE_CAP 10

typedef struct Value Value;
typedef Da(Value *)  Values;

typedef struct Instr Instr;
typedef Da(Instr)    Instrs;

typedef struct {
  Str      name;
  Value   *value;
} Var;

typedef Da(Var) Vars;

typedef struct StackFrame StackFrame;

struct StackFrame {
  Values      values;
  Arena       arena;
  Vars        vars;
  bool        can_lookup_through;
  StackFrame *next;
  StackFrame *prev;
};

typedef struct {
  Args   args;
  Instrs instrs;
} Func;

typedef Da(Func) Ir;

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

typedef struct ListNode  ListNode;
typedef struct DictValue DictValue;

struct ListNode {
  Value    *value;
  ListNode *next;
};

struct DictValue {
  Value *key;
  Value *value;
  DictValue *next;
};

typedef struct {
  DictValue *items[DICT_HASH_TABLE_CAP];
} Dict;

typedef struct {
  Str  str;
} String;

typedef struct {
  Str    name;
  Value *value;
} NamedValue;

typedef Da(NamedValue) NamedValues;

typedef struct FuncValue FuncValue;

struct FuncValue {
  Args         args;
  u32          body_index;
  NamedValues  catched_values_names;
  Func        *parent_func;
  Str          intrinsic_name;
  u32          refs_count;
  bool         cloned;
};

typedef struct Env Env;

typedef struct {
  u8  *ptr;
  u32  len;
} Bytes;

typedef union {
  ListNode  *list;
  String     string;
  i64        _int;
  f64        _float;
  bool       _bool;
  Dict       dict;
  FuncValue *func;
  Env       *env;
  Bytes      bytes;
} ValueAs;

struct Value {
  ValueKind   kind;
  ValueAs     as;
  StackFrame *frame;
  u16         refs_count;
  u16         is_atom;
  Str         parent_var_name;
};

typedef enum {
  InstrKindPrimitive = 0,
  InstrKindFuncCall,
  InstrKindDefVar,
  InstrKindGetVar,
  InstrKindJump,
  InstrKindCondJump,
  InstrKindCondNotJump,
  InstrKindLabel,
  InstrKindMatchBegin,
  InstrKindMatchCase,
  InstrKindMatchEnd,
  InstrKindGet,
  InstrKindSet,
  InstrKindRet,
  InstrKindList,
  InstrKindDict,
  InstrKindSelf,
} InstrKind;

typedef struct {
  Value *value;
} InstrPrimitive;

typedef struct {
  u32    args_len;
  bool   value_ignored;
} InstrFuncCall;

typedef struct {
  Str name;
} InstrDefVar;

typedef struct {
  Str name;
} InstrGetVar;

typedef struct {
  Str label;
} InstrJump;

typedef struct {
  Str label;
} InstrCondJump;

typedef struct {
  Str label;
} InstrCondNotJump;

typedef struct {
  Str name;
} InstrLabel;

typedef struct {
  Str not_label;
} InstrMatchCase;

typedef struct {
  u32 chain_len;
} InstrGet;

typedef struct {
  u32  chain_len;
} InstrSet;

typedef struct {
  bool has_value;
} InstrRet;

typedef struct {
  u32 len;
} InstrList;

typedef struct {
  u32 len;
} InstrDict;

typedef union {
  InstrPrimitive   primitive;
  InstrFuncCall    func_call;
  InstrDefVar      def_var;
  InstrGetVar      get_var;
  InstrJump        jump;
  InstrCondJump    cond_jump;
  InstrCondNotJump cond_not_jump;
  InstrLabel       label;
  InstrMatchCase   match_case;
  InstrGet         get;
  InstrSet         set;
  InstrRet         ret;
  InstrList        list;
  InstrDict        dict;
} InstrAs;

typedef struct {
  Str *file_path;
  u16  row, col;
} InstrMeta;

struct Instr {
  InstrKind kind;
  InstrAs   as;
  InstrMeta meta;
};

typedef struct Vm Vm;
typedef struct Expr Expr;
typedef Da(Expr *) Exprs;

typedef enum {
  ExprKindPrimitive = 0,
  ExprKindBlock,
  ExprKindIdent,
  ExprKindFunc,
  ExprKindList,
  ExprKindDict,
  ExprKindGet,
  ExprKindSet,
  ExprKindFuncCall,
  ExprKindLet,
  ExprKindRet,
  ExprKindIf,
  ExprKindWhile,
  ExprKindMatch,
  ExprKindSelf,
} ExprKind;

typedef struct  {
  Value *value;
} ExprPrimitive;

typedef struct  {
  Str name;
} ExprIdent;

typedef struct {
  Args  args;
  Exprs body;
  Str   intrinsic_name;
} ExprFunc;

typedef struct  {
  Exprs content;
} ExprList;

typedef struct  {
  Exprs content;
} ExprDict;

typedef struct  {
  Exprs chain;
} ExprGet;

typedef struct  {
  Exprs  chain;
  Expr  *new;
} ExprSet;

typedef struct {
  Expr  *func;
  Exprs  args;
  bool   value_ignored;
} ExprFuncCall;

typedef struct {
  Str   name;
  Expr *value;
} ExprLet;

typedef struct {
  Expr *value;
} ExprRet;

typedef struct {
  Expr  *cond;
  Exprs  if_body;
  Exprs  else_body;
} ExprIf;

typedef struct {
  Expr   *value;
  Exprs   values;
  Exprs   branches;
  Expr   *else_branch;
} ExprMatch;

typedef struct {
  Expr  *cond;
  Exprs  body;
} ExprWhile;

typedef union {
  ExprPrimitive primitive;
  Exprs         block;
  ExprIdent     ident;
  ExprFunc      func;
  ExprList      list;
  ExprDict      dict;
  ExprGet       get;
  ExprSet       set;
  ExprFuncCall  func_call;
  ExprLet       let;
  ExprRet       ret;
  ExprIf        _if;
  ExprMatch     match;
  ExprWhile     _while;
} ExprAs;

struct Expr {
  ExprKind  kind;
  ExprAs    as;
  InstrMeta meta;
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
Value *value_func(FuncValue *func, StackFrame *frame);
Value *value_env(Vm *vm, StackFrame *frame);
Value *value_bytes(Bytes bytes, StackFrame *frame);

Value *value_alloc(StackFrame *frame);
Value *value_clone(Value *value, StackFrame *frame);
void   value_free(Value *value);
bool   value_eq(Value *a, Value *b);

void frame_free(StackFrame *frame);

Exprs ast_clone(Exprs *ast, Arena *arena);
Ir    ast_to_ir(Exprs *ast, Arena *arena);

#endif // BYTECODE_VM
