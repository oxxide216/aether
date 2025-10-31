#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <errno.h>

#include "aether/vm.h"
#include "aether/misc.h"
#include "io.h"

bool unblock_input_intrinsic(Vm *vm) {
  (void) vm;

  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

  return true;
}

bool block_input_intrinsic(Vm *vm) {
  (void) vm;

  fcntl(0, F_SETFL, fcntl(0, F_GETFL) ^ O_NONBLOCK);

  return true;
}

static char *str_to_cstr(Str str) {
  char *cstr = malloc(str.len + 1);
  memcpy(cstr, str.ptr, str.len);
  cstr[str.len] = '\0';

  return cstr;
}

bool get_file_info_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = str_to_cstr(path->as.string);

  if (access(path_cstring, F_OK) != 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(path_cstring);

    return true;
  }

  Dict info = {0};

  DIR *directory = opendir(path_cstring);

  Value *is_directory = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  is_directory->kind = ValueKindBool;
  is_directory->as._bool = directory != NULL || errno != ENOTDIR;
  dict_push_value_str_key(&vm->rc_arena, &info,
                          STR_LIT("is-directory"),
                          is_directory);

  if (directory)
    closedir(directory);

  struct stat st;

  if (stat(path_cstring, &st) < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(path_cstring);
    return true;
  }

  Value *size = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
  size->kind = ValueKindInt;
  size->as._int = st.st_size;
  dict_push_value_str_key(&vm->rc_arena, &info,
                          STR_LIT("size"),
                          size);

  value_stack_push_dict(&vm->stack, &vm->rc_arena, info);

  free(path_cstring);

  return true;
}

bool read_file_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = str_to_cstr(path->as.string);

  Str content = read_file(path_cstring);
  if (content.len == (u32) -1)
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
  else
    value_stack_push_string(&vm->stack, &vm->rc_arena, content);

  free(path_cstring);

  return true;
}

bool write_file_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);
  Value *content = value_stack_pop(&vm->stack);

  char *path_cstring = str_to_cstr(path->as.string);

  write_file(path_cstring, content->as.string);

  free(path_cstring);

  return true;
}

bool delete_file_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = str_to_cstr(path->as.string);

  remove(path_cstring);

  free(path_cstring);

  return true;
}

static i32 unlink_dir_callback(const char *fpath, const struct stat *sb,
                               i32 typeflag, struct FTW *ftwbuf) {
  (void) sb;
  (void) typeflag;
  (void) ftwbuf;

  return remove(fpath);
}

bool delete_directory_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = str_to_cstr(path->as.string);

  nftw(path_cstring, unlink_dir_callback, 64, FTW_DEPTH | FTW_PHYS);

  return true;
}

bool list_directory_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  ListNode *list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  ListNode *list_end = list;

  char *path_cstring = str_to_cstr(path->as.string);

  DIR *dir = opendir(path_cstring);
  if (dir) {
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
      Str path;
      path.len = strlen(entry->d_name);
      path.ptr = rc_arena_alloc(&vm->rc_arena, path.len);
      memcpy(path.ptr, entry->d_name, path.len);

      list_end->next = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
      list_end = list_end->next;
      list_end->value = rc_arena_alloc(&vm->rc_arena, sizeof(Value));
      list_end->value->kind = ValueKindString;
      list_end->value->as.string = path;
    }

    closedir(dir);
  } else {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, list);

  free(path_cstring);

  return true;
}

Intrinsic io_intrinsics[] = {
  { STR_LIT("unblock-input"), false, 0, {}, &unblock_input_intrinsic },
  { STR_LIT("block-input"), false, 0, {}, &block_input_intrinsic },
  // Files
  { STR_LIT("get-file-info"), true, 1, { ValueKindString }, &get_file_info_intrinsic },
  { STR_LIT("read-file"), true, 1, { ValueKindString }, &read_file_intrinsic },
  { STR_LIT("write-file"), false, 2,
    { ValueKindString, ValueKindString },
    &write_file_intrinsic },
  { STR_LIT("delete-file"), false, 1, { ValueKindString }, &delete_file_intrinsic },
  { STR_LIT("delete-directory"), false, 1, { ValueKindString }, &delete_directory_intrinsic },
  { STR_LIT("list-directory"), true, 1, { ValueKindString }, &list_directory_intrinsic },
};

u32 io_intrinsics_len = ARRAY_LEN(io_intrinsics);
