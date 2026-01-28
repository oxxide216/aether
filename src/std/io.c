#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <errno.h>

#include "aether/vm.h"
#include "aether/misc.h"
#include "aether/io.h"

#ifndef EMSCRIPTEN
Value *unblock_input_intrinsic(Vm *vm, Value **args) {
  (void) args;

  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

  return value_unit(vm->current_frame);
}

Value *block_input_intrinsic(Vm *vm, Value **args) {
  (void) args;

  fcntl(0, F_SETFL, fcntl(0, F_GETFL) ^ O_NONBLOCK);

  return value_unit(vm->current_frame);
}
#endif

static char *str_to_cstr(Str str) {
  char *cstr = malloc(str.len + 1);
  memcpy(cstr, str.ptr, str.len);
  cstr[str.len] = '\0';

  return cstr;
}

Value *get_file_info_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = str_to_cstr(path->as.string.str);

  if (access(path_cstring, F_OK) != 0) {
    free(path_cstring);

    return value_unit(vm->current_frame);
  }

  Dict *info = arena_alloc(&vm->current_frame->arena, sizeof(Dict));

  DIR *directory = opendir(path_cstring);

  Value *is_directory = value_alloc(vm->current_frame);
  is_directory->kind = ValueKindBool;
  is_directory->as._bool = directory != NULL || errno != ENOTDIR;
  Value *is_directory_key = value_string(STR_LIT("is-directory"), vm->current_frame);
  dict_set_value(vm->current_frame, info, is_directory_key, is_directory);

  if (directory)
    closedir(directory);

  struct stat st;

  if (stat(path_cstring, &st) < 0) {
    free(path_cstring);

    return value_unit(vm->current_frame);
  }

  Value *size = value_alloc(vm->current_frame);
  size->kind = ValueKindInt;
  size->as._int = st.st_size;
  Value *size_key = value_string(STR_LIT("size"), vm->current_frame);
  dict_set_value(vm->current_frame, info, size_key, size);

  free(path_cstring);

  return value_dict(info, vm->current_frame);
}

Value *read_file_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = str_to_cstr(path->as.string.str);

  Str content = read_file_arena(path_cstring, &vm->current_frame->arena);

  free(path_cstring);

  if (content.len == (u32) -1)
    return value_unit(vm->current_frame);

  return value_string(content, vm->current_frame);
}

Value *read_binary_file_intrinsic(Vm *vm, Value **args) {
  Value *result = read_file_intrinsic(vm, args);
  if (result->kind == ValueKindUnit)
    return result;

  Bytes bytes = {
    (u8 *) result->as.string.str.ptr,
    result->as.string.str.len,
  };

  return value_bytes(bytes, vm->current_frame);
}

Value *write_file_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];
  Value *content = args[1];

  Str data = {0};

  if (content->kind == ValueKindString) {
    data = (Str) {
      content->as.string.str.ptr,
      content->as.string.str.len,
    };
  } else if (content->kind == ValueKindBytes) {
    data = (Str) {
      (char *) content->as.bytes.ptr,
      content->as.bytes.len,
    };
  }

  char *path_cstring = str_to_cstr(path->as.string.str);

  bool success = write_file(path_cstring, data);

  free(path_cstring);

  return value_bool(success, vm->current_frame);
}

Value *delete_file_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  char *path_cstring = str_to_cstr(path->as.string.str);

  remove(path_cstring);

  free(path_cstring);

  return value_unit(vm->current_frame);
}

static i32 unlink_dir_callback(const char *fpath, const struct stat *sb,
                               i32 typeflag, struct FTW *ftwbuf) {
  (void) sb;
  (void) typeflag;
  (void) ftwbuf;

  return remove(fpath);
}

Value *delete_directory_intrinsic(Vm *vm, Value **args) {
  (void) args;

  Value *path = args[0];

  char *path_cstring = str_to_cstr(path->as.string.str);

  nftw(path_cstring, unlink_dir_callback, 64, FTW_DEPTH | FTW_PHYS);

  return value_unit(vm->current_frame);
}

Value *list_directory_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];

  ListNode *list = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
  ListNode *list_end = list;

  char *path_cstring = str_to_cstr(path->as.string.str);

  DIR *dir = opendir(path_cstring);
  if (dir) {
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
      Str path;
      path.len = strlen(entry->d_name);
      path.ptr = arena_alloc(&vm->current_frame->arena, path.len);
      memcpy(path.ptr, entry->d_name, path.len);

      list_end->next = arena_alloc(&vm->current_frame->arena, sizeof(ListNode));
      list_end = list_end->next;
      list_end->value = value_string(path, vm->current_frame);
    }

    closedir(dir);
  } else {
    free(path_cstring);

    return value_unit(vm->current_frame);
  }

  free(path_cstring);

  return value_list(list, vm->current_frame);
}

Intrinsic io_intrinsics[] = {
#ifndef EMSCRIPTEN
  { STR_LIT("unblock-input"), false, 0, {}, &unblock_input_intrinsic, NULL },
  { STR_LIT("block-input"), false, 0, {}, &block_input_intrinsic, NULL },
#endif
  // Files
  { STR_LIT("get-file-info"), true, 1, { ValueKindString }, &get_file_info_intrinsic, NULL },
  { STR_LIT("read-file"), true, 1, { ValueKindString }, &read_file_intrinsic, NULL },
  { STR_LIT("read-binary-file"), true, 1, { ValueKindString }, &read_binary_file_intrinsic, NULL },
  { STR_LIT("write-file"), false, 2,
    { ValueKindString, ValueKindString },
    &write_file_intrinsic, NULL },
  { STR_LIT("write-binary-file"), true, 2,
    { ValueKindString, ValueKindBytes },
    &write_file_intrinsic, NULL },
  { STR_LIT("delete-file"), false, 1, { ValueKindString }, &delete_file_intrinsic, NULL },
  { STR_LIT("delete-directory"), false, 1, { ValueKindString }, &delete_directory_intrinsic, NULL },
  { STR_LIT("list-directory"), true, 1, { ValueKindString }, &list_directory_intrinsic, NULL },
};

u32 io_intrinsics_len = ARRAY_LEN(io_intrinsics);
