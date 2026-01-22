#include "aether/deserializer.h"
#include "aether/serializer.h"
#include "aether/common.h"
#include "shl/shl-log.h"

static void deserialize_str(Str *str, u8 *data, u32 *end, Arena *arena) {
  str->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  str->ptr = arena_alloc(arena, str->len);
  for (u32 i = 0; i < str->len; ++i) {
    str->ptr[i] = *(char *) (data + *end);
    *end += 1;
  }
}

static void deserialize_instrs(Instrs *instrs, u8 *data, u32 *end,
                               FilePathOffsets *path_offsets,
                               Arena *arena) {
  instrs->len = *(u32 *) (data + *end);
  *end += sizeof(u32);

  for (u32 i = 0; i < instrs->len; ++i) {
    Instr instr = {0};

    ERROR("TODO: deserialization\n");
    exit(1);

    DA_APPEND(*instrs, instr);
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
    deserialize_str(&path_offsets->items[i].path, data, end, arena);
  }
}

Ir deserialize(u8 *data, u32 size, Arena *arena, Str *file_path) {
  Ir ir = {0};

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

  ir.len = *(u32 *) (data + end);
  end += sizeof(u32);

  for (u32 i = 0; i < ir.len; ++i) {
    Func func = {0};

    deserialize_instrs(&func.instrs, data, &end, &path_offsets, arena);

    DA_APPEND(ir, func);
  }

  return ir;
}

static void deserialize_ast(Exprs *exprs, u8 *data, u32 *end,
                            FilePathOffsets *path_offsets,
                            Arena *arena) {
  ERROR("TODO: deserialization\n");
  exit(1);
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

    deserialize_str(&macro->name, data, &end, arena);

    macro->arg_names.len = *(u32 *) (data + end);
    end += sizeof(u32);

    macro->arg_names.items = arena_alloc(arena, macro->arg_names.len * sizeof(Str));
    for (u32 j = 0; j < macro->arg_names.len; ++j)
      deserialize_str(macro->arg_names.items + j, data, &end, arena);

    deserialize_ast(&macro->body, data, &end, &path_offsets, arena);

    macro->has_unpack = *(u8 *) (data + end);
    end += sizeof(u8);
  }

  return macros;
}
