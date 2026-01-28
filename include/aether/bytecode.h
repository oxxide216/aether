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
  Values      match_values;
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

typedef struct FuncValue FuncValue;

struct FuncValue {
  Args  args;
  u32   body_index;
  Vars  catched_vars;
  Func *parent_func;
  u16   intrinsic_name_id;
  u32   refs_count;
  bool  cloned;
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
  Dict      *dict;
  FuncValue *func;
  Env       *env;
  Bytes      bytes;
} ValueAs;

struct Value {
  ValueKind    kind;
  ValueAs      as;
  StackFrame  *frame;
  u16          refs_count;
  u16          is_atom;
};

typedef struct Expr Expr;
typedef Da(Expr *) DaExprs;

typedef struct {
  Expr **items;
  u32    len;
} Exprs;

typedef struct {
  Expr *value;
  Expr *body;
} Branch;

typedef Da(Branch) Branches;

typedef enum {
  InstrKindString = 0,
  InstrKindInt,
  InstrKindFloat,
  InstrKindBytes,
  InstrKindFunc,
  InstrKindFuncCall,
  InstrKindDefVar,
  InstrKindGetVar,
  InstrKindSetVar,
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
  u16 string_id;
} InstrString;

typedef struct {
  i64 _int;
} InstrInt;

typedef struct {
  f64 _float;
} InstrFloat;

typedef struct {
  Bytes bytes;
} InstrBytes;

typedef struct {
  Args args;
  u16  body_index;
  u16  intrinsic_name_id;
} InstrFunc;

typedef struct {
  u32    args_len;
  bool   value_ignored;
} InstrFuncCall;

typedef struct {
  u16 name_id;
} InstrDefVar;

typedef struct {
  u16 name_id;
} InstrGetVar;

typedef struct {
  u16 name_id;
} InstrSetVar;

typedef struct {
  u16 label_id;
} InstrJump;

typedef struct {
  u16 label_id;
} InstrCondJump;

typedef struct {
  u16 label_id;
} InstrCondNotJump;

typedef struct {
  u16 name_id;
} InstrLabel;

typedef struct {
  u16 not_label_id;
} InstrMatchCase;

typedef struct {
  u32 chain_len;
} InstrGet;

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
  InstrString      string;
  InstrInt         _int;
  InstrFloat       _float;
  InstrBytes       bytes;
  InstrFunc        func;
  InstrFuncCall    func_call;
  InstrDefVar      def_var;
  InstrGetVar      get_var;
  InstrSetVar      set_var;
  InstrJump        jump;
  InstrCondJump    cond_jump;
  InstrCondNotJump cond_not_jump;
  InstrLabel       label;
  InstrMatchCase   match_case;
  InstrGet         get;
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

typedef enum {
  ExprKindString = 0,
  ExprKindInt,
  ExprKindFloat,
  ExprKindBytes,
  ExprKindBlock,
  ExprKindIdent,
  ExprKindFunc,
  ExprKindList,
  ExprKindDict,
  ExprKindGet,
  ExprKindSet,
  ExprKindSetVar,
  ExprKindFuncCall,
  ExprKindLet,
  ExprKindRet,
  ExprKindIf,
  ExprKindWhile,
  ExprKindMatch,
  ExprKindSelf,
} ExprKind;

typedef struct  {
  u16 string_id;
} ExprString;

typedef struct  {
  i64 _int;
} ExprInt;

typedef struct  {
  f64 _float;
} ExprFloat;

typedef struct  {
  Bytes bytes;
} ExprBytes;

typedef struct  {
  u16 name_id;
} ExprIdent;

typedef struct {
  Args  args;
  Exprs body;
  u16   intrinsic_name_id;
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
  Expr  *parent;
  Expr  *key;
  Expr  *new;
} ExprSet;

typedef struct  {
  u16   name_id;
  Expr *new;
} ExprSetVar;

typedef struct {
  Expr  *func;
  Exprs  args;
  bool   value_ignored;
} ExprFuncCall;

typedef struct {
  u16   name_id;
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
  Expr     *value;
  Branches  branches;
} ExprMatch;

typedef struct {
  Expr  *cond;
  Exprs  body;
} ExprWhile;

typedef union {
  ExprString   string;
  ExprInt      _int;
  ExprFloat    _float;
  ExprBytes    bytes;
  Exprs        block;
  ExprIdent    ident;
  ExprFunc      func;
  ExprList     list;
  ExprDict     dict;
  ExprGet      get;
  ExprSet      set;
  ExprSetVar   set_var;
  ExprFuncCall func_call;
  ExprLet      let;
  ExprRet      ret;
  ExprIf       _if;
  ExprMatch    match;
  ExprWhile    _while;
} ExprAs;

struct Expr {
  ExprKind  kind;
  ExprAs    as;
  InstrMeta meta;
};

ListNode *list_clone(ListNode *list, StackFrame *frame);
Dict     *dict_clone(Dict *dict, StackFrame *frame);

Value *value_unit(StackFrame *frame);
Value *value_list(ListNode *nodes, StackFrame *frame);
Value *value_string(Str string, StackFrame *frame);
Value *value_int(i64 _int, StackFrame *frame);
Value *value_float(f64 _float, StackFrame *frame);
Value *value_bool(bool _bool, StackFrame *frame);
Value *value_dict(Dict *dict, StackFrame *frame);
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
