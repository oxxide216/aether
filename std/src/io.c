#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>

#include "aether/vm.h"
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

bool file_exists_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  value_stack_push_bool(&vm->stack, &vm->rc_arena, access(path_cstring, F_OK) == 0);

  free(path_cstring);

  return true;
}

bool read_file_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

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

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  write_file(path_cstring, content->as.string);

  free(path_cstring);

  return true;
}

bool delete_file_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

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

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

  nftw(path_cstring, unlink_dir_callback, 64, FTW_DEPTH | FTW_PHYS);

  return true;
}

bool list_directory_intrinsic(Vm *vm) {
  Value *path = value_stack_pop(&vm->stack);

  ListNode *list = rc_arena_alloc(&vm->rc_arena, sizeof(ListNode));
  ListNode *list_end = list;

  char *path_cstring = malloc(path->as.string.len + 1);
  memcpy(path_cstring, path->as.string.ptr, path->as.string.len);
  path_cstring[path->as.string.len] = '\0';

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
  }

  value_stack_push_list(&vm->stack, &vm->rc_arena, list);

  free(path_cstring);

  return true;
}

Intrinsic io_intrinsics[] = {
  { STR_LIT("unblock-input"), false, 0, {}, &unblock_input_intrinsic },
  { STR_LIT("block-input"), false, 0, {}, &block_input_intrinsic },
  // Files
  { STR_LIT("file-exists"), true, 1, { ValueKindString }, &file_exists_intrinsic },
  { STR_LIT("read-file"), true, 1, { ValueKindString }, &read_file_intrinsic },
  { STR_LIT("write-file"), false, 2,
    { ValueKindString, ValueKindString },
    &write_file_intrinsic },
  { STR_LIT("delete-file"), false, 1, { ValueKindString }, &delete_file_intrinsic },
  { STR_LIT("delete-directory"), false, 1, { ValueKindString }, &delete_directory_intrinsic },
  { STR_LIT("list-directory"), true, 1, { ValueKindString }, &list_directory_intrinsic },
};

u32 io_intrinsics_len = ARRAY_LEN(io_intrinsics);
