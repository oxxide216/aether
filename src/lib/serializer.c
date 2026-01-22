#include "aether/serializer.h"
#include "aether/optimizer.h"
#include "shl/shl-log.h"

static void reserve_space(u32 amount, u8 **data, u32 *data_size, u32 *end) {
  u32 new_data_size = *data_size;
  while (*end + amount > new_data_size)
    new_data_size *= 2;

  if (new_data_size != *data_size) {
    *data_size = new_data_size;
    *data = realloc(*data, *data_size);
  }
}

static void serialize_str(Str str, u8 **data, u32 *data_size, u32 *end) {
  reserve_space(sizeof(u32) + str.len, data, data_size, end);
  *(u32 *) (*data + *end) = str.len;
  *end += sizeof(u32);

  for (u32 i = 0; i < str.len; ++i) {
    *(char *) (*data + *end) = str.ptr[i];
    *end += sizeof(char);
  }
}

static void serialize_instrs(Instrs *instrs, u8 **data, u32 *data_size,
                             u32 *end, FilePathOffsets *path_offsets,
                             Str *file_path) {
  ERROR("TODO: serialization\n");
  exit(1);
}

static void serialize_included_files(u8 **data, u32 *data_size, u32 *end,
                                     FilePaths *included_files,
                                     FilePathOffsets *path_offsets) {
  reserve_space(sizeof(u32), data, data_size, end);
  *(u32 *) (*data + *end) = included_files->len;
  *end += sizeof(u32);

  for (u32 i = 0; i < included_files->len; ++i) {
    FilePathOffset path_offset = { *included_files->items[i], *end };
    DA_APPEND(*path_offsets, path_offset);

    serialize_str(*included_files->items[i], data, data_size, end);
  }
}

u8 *serialize(Ir *ir, u32 *size, FilePaths *included_files, bool dce) {
  *size = sizeof(u32) * 2;
  u32 data_size = sizeof(u32) * 2;
  u8 *data = malloc(data_size);

  if (dce)
    eliminate_dead_code(ir);

  FilePathOffsets path_offsets = {0};
  serialize_included_files(&data, &data_size, size, included_files, &path_offsets);

  for (u32 i = 0; i < ir->len; ++i) {
    serialize_instrs(&ir->items[i].instrs, &data, &data_size, size,
                     &path_offsets, included_files->items[0]);
  }

  if (path_offsets.items)
    free(path_offsets.items);

  *(u32 *) data = *(u32 *) "ABC\0";
  *(u32 *) (data + sizeof(u32)) = *size;

  return data;
}

static void serialize_ast(Exprs *ast, u8 **data, u32 *data_size,
                          u32 *end, FilePathOffsets *path_offsets,
                          Str *file_path) {
  ERROR("TODO: serialization\n");
  exit(1);
}

u8 *serialize_macros(Macros *macros, u32 *size,
                     FilePaths *included_files, bool dce) {
  *size = sizeof(u32) * 2;
  u32 data_size = sizeof(u32) * 2;
  u8 *data = malloc(data_size);

  FilePathOffsets path_offsets = {0};
  serialize_included_files(&data, &data_size, size, included_files, &path_offsets);

  reserve_space(sizeof(u32), &data, &data_size, size);
  *(u32 *) (data + *size) = macros->len;
  *size += sizeof(u32);

  for (u32 i = 0; i < macros->len; ++i) {
    Macro *macro = macros->items + i;

    serialize_str(macro->name, &data, &data_size, size);

    reserve_space(sizeof(u32), &data, &data_size, size);
    *(u32 *) (data + *size) = macro->arg_names.len;
    *size += sizeof(u32);

    for (u32 i = 0; i < macro->arg_names.len; ++i)
      serialize_str(macro->arg_names.items[i], &data, &data_size, size);

    serialize_ast(&macro->body, &data, &data_size, size,
                  &path_offsets, included_files->items[0]);

    reserve_space(sizeof(u8), &data, &data_size, size);
    *(u8 *) (data + *size) = macro->has_unpack;
    *size += sizeof(u8);
  }

  *(u32 *) data = *(u32 *) "ABM\0";
  *(u32 *) (data + sizeof(u32)) = *size;

  if (path_offsets.items)
    free(path_offsets.items);

  return data;
}
