#include "aether/deserializer.h"
#include "aether/serializer.h"
#include "aether/common.h"
#include "aether/misc.h"
#include "shl/shl-log.h"

static Str deserialize_str_raw(u8 *data, u32 *end, Arena *arena) {
  Str str;

  str.len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  str.ptr = arena_alloc(arena, str.len);
  for (u32 i = 0; i < str.len; ++i) {
    str.ptr[i] = *(char *) (data + *end);
    *end += 1;
  }

  return str;
}

static u16 deserialize_str(u8 *data, u32 *end, Arena *arena) {
  return copy_str(deserialize_str_raw(data, end, arena));
}

static void deserialize_instrs(Instrs *instrs, u8 *data, u32 *end,
                               FilePathOffsets *path_offsets,
                               Arena *arena) {
  instrs->len = *(u32 *) (data + *end);
  instrs->cap = instrs->len;
  *end += sizeof(u32);

  instrs->items = malloc(instrs->cap * sizeof(Instr));

  for (u32 i = 0; i < instrs->len; ++i) {
    Instr instr = {0};

    instr.kind = *(InstrKind *) (data + *end);
    *end += sizeof(InstrKind);

    switch (instr.kind) {
    case InstrKindString: {
      instr.as.string.string_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindInt: {
      instr.as._int._int = *(i64 *) (data + *end);
      *end += sizeof(i64);
    } break;

    case InstrKindFloat: {
      instr.as._float._float = *(f64 *) (data + *end);
      *end += sizeof(f64);
    } break;

    case InstrKindBytes: {
      Str bytes_str = deserialize_str_raw(data, end, arena);

      instr.as.bytes.bytes.ptr = (u8 *) bytes_str.ptr;
      instr.as.bytes.bytes.len = bytes_str.len;
    } break;

    case InstrKindFunc: {
      instr.as.func.args.len = *(u32 *) (data + *end);
      *end += sizeof(u32);

      instr.as.func.args.items =
        arena_alloc(arena, instr.as.func.args.len * sizeof(Str));

      for (u32 i = 0; i < instr.as.func.args.len; ++i)
        instr.as.func.args.items[i] = deserialize_str(data, end, arena);

      bool has_intrinsic_name  = *(u8 *) (data + *end);
      *end += sizeof(u8);

      if (has_intrinsic_name) {
        instr.as.func.intrinsic_name_id = deserialize_str(data, end, arena);
      } else {
        instr.as.func.body = arena_alloc(arena, sizeof(Func));
        instr.as.func.body->args = instr.as.func.args;
        deserialize_instrs(&instr.as.func.body->instrs, data, end, path_offsets, arena);
        instr.as.func.intrinsic_name_id = (u16) -1;
      }
    } break;

    case InstrKindFuncCall: {
      instr.as.func_call.args_len = *(u32 *) (data + *end);
      *end += sizeof(u32);
    } break;

    case InstrKindDefVar: {
      instr.as.def_var.name_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindGetVar: {
      instr.as.get_var.name_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindGet: {
      instr.as.get.chain_len = *(u32 *) (data + *end);
      *end += sizeof(u32);
    } break;

    case InstrKindSet: {
      instr.as.set.name_id = deserialize_str(data, end, arena);

      instr.as.set.chain_len = *(u32 *) (data + *end);
      *end += sizeof(u32);
    } break;

    case InstrKindJump: {
      instr.as.jump.label_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindCondJump: {
      instr.as.cond_jump.label_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindCondNotJump: {
      instr.as.cond_not_jump.label_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindLabel: {
      instr.as.label.name_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindMatchBegin: break;

    case InstrKindMatchCase: {
      instr.as.match_case.not_label_id = deserialize_str(data, end, arena);
    } break;

    case InstrKindMatchEnd: break;

    case InstrKindRet: {
      instr.as.ret.has_value = *(u8 *) (data + *end);
      *end += sizeof(u8);
    } break;

    case InstrKindList: {
      instr.as.list.len = *(u32 *) (data + *end);
      *end += sizeof(u32);
    } break;

    case InstrKindDict: {
      instr.as.dict.len = *(u32 *) (data + *end);
      *end += sizeof(u32);
    } break;

    case InstrKindSelf: break;
    }

    u32 path_index = *(u32 *) (data + *end);
    *end += sizeof(u32);

    if (path_index >= path_offsets->len) {
      ERROR("Offset index out of bounds: %u\n", path_index);
      exit(1);
    }

    instr.meta.file_path = &path_offsets->items[path_index].path;
    instr.meta.row = *(u16 *) (data + *end);
    *end += sizeof(u16);
    instr.meta.col = *(u16 *) (data + *end);
    *end += sizeof(u16);

    instrs->items[i] = instr;
  }
}

static void deserialize_path_offsets(FilePathOffsets *path_offsets,
                                     u8 *data, u32 *end, Arena *arena) {
  path_offsets->len = *(u32 *) (data + *end);
  path_offsets->cap = path_offsets->len;
  *end += sizeof(u32);

  path_offsets->items = arena_alloc(arena, path_offsets->cap * sizeof(FilePathOffset));
  for (u32 i = 0; i < path_offsets->len; ++i) {
    path_offsets->items[i].offset = *end;
    path_offsets->items[i].path = deserialize_str_raw(data, end, arena);
  }
}

Ir deserialize(u8 *data, u32 size, Arena *arena, Str *file_path) {
  Ir ir = arena_alloc(arena, sizeof(Func));

  if (size < sizeof(u32) * 3) {
    ERROR("Corrupted bytecode: not enough data\n");
    exit(1);
  }

  if (!str_eq(STR((char *) data, 4), STR_LIT("ABC\0"))) {
    ERROR("Corrupted bytecode: wrong magic\n");
    exit(1);
  }

  u32 expected_size = *(u32 *) (data + sizeof(u32));
  if (size != expected_size) {
    ERROR("Corrupted bytecode: expected %u, but got %u bytes\n",
          expected_size, size);
    exit(1);
  }

  u32 end = sizeof(u32) * 2;

  FilePathOffsets path_offsets = {0};
  deserialize_path_offsets(&path_offsets, data, &end, arena);

  *file_path = path_offsets.items[0].path;

  ir->args.len = *(u32 *) (data + end);
  ir->args.cap = ir->args.len;
  end += sizeof(u32);

  ir->args.items = arena_alloc(arena, ir->args.cap * sizeof(Str));
  for (u32 i = 0; i < ir->args.len; ++i)
    ir->args.items[i] = deserialize_str(data, &end, arena);

  deserialize_instrs(&ir->instrs, data, &end, &path_offsets, arena);

  return ir;
}

static void deserialize_ast(Exprs *ast, u8 *data, u32 *end,
                            FilePathOffsets *path_offsets,
                            Arena *arena);

static Expr *deserialize_ast_node(u8 *data, u32 *end,
                                  FilePathOffsets *path_offsets,
                                  Arena *arena) {
  Expr *node = arena_alloc(arena, sizeof(Expr));

  node->kind = *(ExprKind *) (data + *end);
  *end += sizeof(ExprKind);

  switch (node->kind) {
  case ExprKindString: {
    node->as.string.string_id = deserialize_str(data, end, arena);
  } break;

  case ExprKindInt: {
    node->as._int._int = *(i64 *) (data + *end);
    *end += sizeof(i64);
  } break;

  case ExprKindFloat: {
    node->as._float._float = *(f64 *) (data + *end);
    *end += sizeof(f64);
  } break;

  case ExprKindBytes: {
    Str bytes_str = deserialize_str_raw(data, end, arena);

    node->as.bytes.bytes.ptr = (u8 *) bytes_str.ptr;
    node->as.bytes.bytes.len = bytes_str.len;
  } break;

  case ExprKindBlock: {
    deserialize_ast(&node->as.block, data,
                    end, path_offsets, arena);
  } break;

  case ExprKindIdent: {
    node->as.ident.name_id = deserialize_str(data, end, arena);
  } break;

  case ExprKindFunc: {
    node->as.func.args.len = *(u32 *) (data + *end);
    node->as.func.args.cap = node->as.func.args.len;
    *end += sizeof(u32);

    node->as.func.args.items =
      arena_alloc(arena, node->as.func.args.cap * sizeof(Str));

    for (u32 j = 0; j < node->as.func.args.len; ++j)
      node->as.func.args.items[j] = deserialize_str(data, end, arena);

    deserialize_ast(&node->as.func.body, data,
                    end, path_offsets, arena);

    bool has_intrinsic_name  = *(u8 *) (data + *end);
    *end += sizeof(u8);

    if (has_intrinsic_name)
      node->as.func.intrinsic_name_id = deserialize_str(data, end, arena);
    else
      node->as.func.intrinsic_name_id = (u16) -1;
  } break;

  case ExprKindList: {
    deserialize_ast(&node->as.list.content, data,
                    end, path_offsets, arena);
  } break;

  case ExprKindDict: {
    deserialize_ast(&node->as.dict.content, data,
                    end, path_offsets, arena);
  } break;

  case ExprKindGet: {
    deserialize_ast(&node->as.get.chain, data,
                    end, path_offsets, arena);
  } break;

  case ExprKindSet: {
    node->as.set.name_id = deserialize_str(data, end, arena);

    deserialize_ast(&node->as.set.chain, data,
                    end, path_offsets, arena);
    node->as.set.new = deserialize_ast_node(data, end,
                                            path_offsets, arena);
  } break;

  case ExprKindFuncCall: {
    node->as.func_call.func = deserialize_ast_node(data, end,
                                                   path_offsets, arena);
    deserialize_ast(&node->as.func_call.args, data,
                    end, path_offsets, arena);
  } break;

  case ExprKindLet: {
    node->as.let.name_id = deserialize_str(data, end, arena);
    node->as.let.value = deserialize_ast_node(data, end,
                                              path_offsets, arena);
  } break;

  case ExprKindRet: {
    bool has_value = *(u8 *) (data + *end);
    *end += sizeof(u8);

    if (has_value)
      node->as.ret.value = deserialize_ast_node(data, end,
                                                path_offsets, arena);
  } break;

  case ExprKindIf: {
    node->as._if.cond = deserialize_ast_node(data, end,
                                             path_offsets, arena);
    deserialize_ast(&node->as._if.if_body, data, end,
                    path_offsets, arena);
    deserialize_ast(&node->as._if.else_body, data, end,
                    path_offsets, arena);
  } break;

  case ExprKindMatch: {
    node->as.match.value = deserialize_ast_node(data, end,
                                                path_offsets, arena);

    Branches branches;
    branches.len = *(u32 *) (data + *end);
    branches.cap = branches.len;
    *end += sizeof(u32);

    branches.items = arena_alloc(arena, branches.cap * sizeof(Branch));

    for (u32 i = 0; i < branches.len; ++i) {
      branches.items[i].value = deserialize_ast_node(data, end, path_offsets, arena);
      branches.items[i].body = deserialize_ast_node(data, end, path_offsets, arena);
    }

    node->as.match.branches = branches;
  } break;

  case ExprKindSelf: break;
  }

  u32 path_index = *(u32 *) (data + *end);
  *end += sizeof(u32);

  if (path_index >= path_offsets->len) {
    ERROR("Offset index out of bounds: %u\n", path_index);
    exit(1);
  }

  node->meta.file_path = &path_offsets->items[path_index].path;
  node->meta.row = *(u16 *) (data + *end);
  *end += sizeof(u16);
  node->meta.col = *(u16 *) (data + *end);
  *end += sizeof(u16);

  return node;
}

static void deserialize_ast(Exprs *ast, u8 *data, u32 *end,
                            FilePathOffsets *path_offsets,
                            Arena *arena) {
  ast->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  ast->items = arena_alloc(arena, ast->len * sizeof(Expr *));

  for (u32 i = 0; i < ast->len; ++i)
    ast->items[i] = deserialize_ast_node(data, end, path_offsets, arena);
}

Macros deserialize_macros(u8 *data, u32 size,
                          FilePaths *included_files,
                          Arena *arena) {
  Macros macros = {0};

  if (size < sizeof(u32)) {
    ERROR("Corrupted bytecode: not enough data\n");
    exit(1);
  }

  if (!str_eq(STR((char *) data, 4), STR_LIT("ABM\0"))) {
    ERROR("Corrupted bytecode: wrong magic\n");
    exit(1);
  }

  u32 expected_size = *(u32 *) (data + sizeof(u32));
  if (size != expected_size) {
    ERROR("Corrupted bytecode: expected %u, but got %u bytes\n",
          expected_size, size);
    exit(1);
  }

  u32 end = sizeof(u32) * 2;

  FilePathOffsets path_offsets = {0};
  deserialize_path_offsets(&path_offsets, data, &end, arena);

  if (included_files) {
    if (included_files->cap < included_files->len + path_offsets.len) {
      included_files->cap = included_files->len + path_offsets.len;
      if (included_files->len == 0)
        included_files->items = malloc(included_files->cap * sizeof(Str));
      else
        included_files->items = realloc(included_files->items,
                                        included_files->cap * sizeof(Str));
    }

    for (u32 i = 0; i < path_offsets.len; ++i) {
      Str *new_file_path;
      new_file_path = arena_alloc(arena, sizeof(Str));
      new_file_path->len = path_offsets.items[i].path.len;
      new_file_path->ptr = arena_alloc(arena, new_file_path->len);
      memcpy(new_file_path->ptr, path_offsets.items[i].path.ptr, new_file_path->len);

      included_files->items[included_files->len++] = new_file_path;
    }
  }

  macros.len = *(u32 *) (data + end);
  macros.cap = macros.len;
  end += sizeof(u32);

  macros.items = arena_alloc(arena, macros.cap * sizeof(Macro));

  for (u32 i = 0; i < macros.len; ++i) {
    Macro *macro = macros.items + i;

    macro->name_id = deserialize_str(data, &end, arena);

    macro->arg_names.len = *(u32 *) (data + end);
    end += sizeof(u32);

    macro->arg_names.items = arena_alloc(arena, macro->arg_names.len * sizeof(Str));
    for (u32 j = 0; j < macro->arg_names.len; ++j)
      macro->arg_names.items[j] = deserialize_str(data, &end, arena);

    deserialize_ast(&macro->body, data, &end, &path_offsets, arena);

    macro->has_unpack = *(u8 *) (data + end);
    end += sizeof(u8);
  }

  return macros;
}
