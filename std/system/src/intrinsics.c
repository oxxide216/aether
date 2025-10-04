#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <dirent.h>

#include "system/intrinsics.h"
#include "io.h"

#define DEFAULT_INPUT_BUFFER_SIZE   64
#define DEFAULT_RECEIVE_BUFFER_SIZE 64

static void print_value(ValueStack *stack, Value *value, u32 level) {
  switch (value->kind) {
  case ValueKindUnit: {
    fputs("unit", stdout);
  } break;

  case ValueKindList: {
    fputc('[', stdout);

    ListNode *node = value->as.list->next;
    while (node) {
      if (node != value->as.list->next)
        fputc(' ', stdout);

      if (node->value.kind == ValueKindString)
        fputc('\'', stdout);
      print_value(stack, &node->value, level);
      if (node->value.kind == ValueKindString)
        fputc('\'', stdout);

      node = node->next;
    }

    fputc(']', stdout);
  } break;

  case ValueKindString: {
    str_print(value->as.string);
  } break;

  case ValueKindInt: {
    printf("%ld", value->as._int);
  } break;

  case ValueKindFloat: {
    printf("%f", value->as._float);
  } break;

  case ValueKindBool: {
    if (value->as._bool)
      fputs("true", stdout);
    else
      fputs("false", stdout);
  } break;

  case ValueKindFunc: {
    fputs("(fun [", stdout);
    fputs(" [", stdout);

    for (u32 i = 0; i < value->as.func.args.len; ++i) {
      if (i > 0)
        fputc(' ', stdout);
      str_print(value->as.func.args.items[i]);
    }

    fputs("])", stdout);
  } break;

  case ValueKindRecord: {
    fputs("\n{\n", stdout);

    for (u32 i = 0; i < value->as.record.len; ++i) {
      for (u32 j = 0; j < level + 1; ++j)
        fputs("  ", stdout);

      str_print(value->as.record.items[i].name);
      fputs(": ", stdout);
      print_value(stack, &value->as.record.items[i].value, level + 1);

      fputc('\n', stdout);
    }

    for (u32 j = 0; j < level; ++j)
      fputs("  ", stdout);
    fputs("}\n", stdout);
  } break;
  }
}

bool print_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  print_value(&vm->stack, &value, 0);

  return true;
}

bool println_intrinsic(Vm *vm) {
  print_intrinsic(vm);
  fputc('\n', stdout);

  return true;
}

bool input_intrinsic(Vm *vm) {
  (void) vm;

  u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
  char *buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
  u32 len = 0;

  char ch;
  while ((ch = getc(stdin)) != EOF && ch != '\n') {
    if (len >= buffer_size) {
      buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

      char *prev_buffer = buffer;
      buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
      memcpy(buffer, prev_buffer, len);
      rc_arena_free(vm->rc_arena, prev_buffer);
    }

    buffer[len++] = ch;
  }

  value_stack_push_string(&vm->stack, STR(buffer, len));

  return true;
}

bool file_exists_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("file-exists?: wrong argument kind\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  value_stack_push_bool(&vm->stack, access(path_cstring, F_OK) == 0);

  free(path_cstring);

  return true;
}

bool read_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("read-file: wrong argument kind\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  Str content = read_file(path_cstring);
  if (content.len == (u32) -1)
    value_stack_push_unit(&vm->stack);
  else
    value_stack_push_string(&vm->stack, content);

  free(path_cstring);

  return true;
}

bool write_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  Value content = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString ||
      content.kind != ValueKindString)
    PANIC("write-file: wrong argument kinds\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  write_file(path_cstring, content.as.string);

  free(path_cstring);

  return true;
}

bool delete_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("delete-file: wrong argument kind\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  remove(path_cstring);

  free(path_cstring);

  return true;
}

bool get_current_path_intrinsic(Vm *vm) {
  char *path = rc_arena_alloc(vm->rc_arena, PATH_MAX);
  getcwd(path, PATH_MAX);

  value_stack_push_string(&vm->stack, STR(path, strlen(path)));

  return true;
}

bool set_current_path_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("set-current-path: wrong argument kind\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  chdir(path_cstring);

  free(path_cstring);

  return true;
}

bool get_absolute_path_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("get-absolute-path: wrong argument kind\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  char *absolute_path = rc_arena_alloc(vm->rc_arena, PATH_MAX);
  realpath(path_cstring, absolute_path);

  value_stack_push_string(&vm->stack, STR(absolute_path, strlen(absolute_path)));

  free(path_cstring);

  return true;
}

bool list_directory_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("list-directory: wrong argument kind\n");

  ListNode *list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode *list_end = list;

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  DIR *dir = opendir(path_cstring);
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      LL_PREPEND(list, list_end, ListNode);
      list_end->value.kind = ValueKindString;
      list_end->value.as.string = (Str) {
        entry->d_name,
        strlen(entry->d_name),
      };
    }
    closedir(dir);
  }

  value_stack_push_list(&vm->stack, list);

  return true;
}

bool get_args_intrinsic(Vm *vm) {
  value_stack_push_list(&vm->stack, vm->args);

  return true;
}

bool create_server_intrinsic(Vm *vm) {
  Value port = value_stack_pop(&vm->stack);
  if (port.kind != ValueKindInt)
    PANIC("create-server: wrong argument kind\n");

  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  i32 enable = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port.as._int);

  if (bind(server_socket, (struct sockaddr*) &address,
           sizeof(address)) < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  if (listen(server_socket, 3) < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  value_stack_push_int(&vm->stack, server_socket);

  return true;
}

bool create_client_intrinsic(Vm *vm) {
  Value port = value_stack_pop(&vm->stack);
  Value server_ip_address = value_stack_pop(&vm->stack);
  if (server_ip_address.kind != ValueKindString ||
      port.kind != ValueKindInt)
    PANIC("create-client: wrong argument kind\n");

  i32 client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port.as._int);

  char *server_ip_address_cstr = malloc(server_ip_address.as.string.len + 1);
  memcpy(server_ip_address_cstr,
         server_ip_address.as.string.ptr,
         server_ip_address.as.string.len);
  server_ip_address_cstr[server_ip_address.as.string.len] = '\0';

  if (inet_pton(AF_INET,
                server_ip_address_cstr,
                &server_address.sin_addr) < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  if (connect(client_socket,
              (struct sockaddr*) &server_address,
              sizeof(server_address)) < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  value_stack_push_int(&vm->stack, client_socket);

  return true;
}

bool accept_connection_intrinsic(Vm *vm) {
  Value port = value_stack_pop(&vm->stack);
  Value server_socket = value_stack_pop(&vm->stack);
  if (server_socket.kind != ValueKindInt ||
      port.kind != ValueKindInt)
    PANIC("accept-connection: wrong argument kind\n");

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port.as._int);

  u32 address_size;
  i32 client_socket = accept(server_socket.as._int,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  value_stack_push_int(&vm->stack, client_socket);

  return true;
}

bool close_connection_intrinsic(Vm *vm) {
  Value client_socket = value_stack_pop(&vm->stack);
  if (client_socket.kind != ValueKindInt)
    PANIC("close-connection: wrong argument kind\n");

  close(client_socket.as._int);

  return true;
}

bool send_intrinsic(Vm *vm) {
  Value message = value_stack_pop(&vm->stack);
  Value receiver = value_stack_pop(&vm->stack);
  if (receiver.kind != ValueKindInt ||
      message.kind != ValueKindString)
    PANIC("send: wrong argument kind\n");

  send(receiver.as._int, message.as.string.ptr, message.as.string.len, 0);

  return true;
}

bool receive_intrinsic(Vm *vm) {
  Value receiver = value_stack_pop(&vm->stack);
  if (receiver.kind != ValueKindInt)
    PANIC("receive: wrong argument kind\n");

  Str buffer;
  buffer.len = DEFAULT_RECEIVE_BUFFER_SIZE;
  buffer.ptr = rc_arena_alloc(vm->rc_arena, buffer.len);

  u32 len;
  while ((len = read(receiver.as._int,
                     buffer.ptr + buffer.len,
                     DEFAULT_RECEIVE_BUFFER_SIZE)) ==
         DEFAULT_RECEIVE_BUFFER_SIZE) {
    u32 prev_len = buffer.len;
    char *prev_ptr = buffer.ptr;

    buffer.len += DEFAULT_RECEIVE_BUFFER_SIZE;
    buffer.ptr = rc_arena_alloc(vm->rc_arena, buffer.len);

    memcpy(buffer.ptr, prev_ptr, prev_len);
    rc_arena_free(vm->rc_arena, prev_ptr);
  }

  value_stack_push_string(&vm->stack, buffer);

  return true;
}

bool run_command_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString)
    PANIC("run-command: wrong argument kind\n");

  char *path_cstring = malloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  value_stack_push_int(&vm->stack, system(path_cstring));

  free(path_cstring);

  return true;
}

Intrinsic system_intrinsics[] = {
  // Io
  { STR_LIT("print"), 1, false, &print_intrinsic },
  { STR_LIT("println"), 1, false, &println_intrinsic },
  { STR_LIT("input"), 0, true, &input_intrinsic },
  // Files
  { STR_LIT("file-exists?"), 1, true, &file_exists_intrinsic },
  { STR_LIT("read-file"), 1, true, &read_file_intrinsic },
  { STR_LIT("write-file"), 2, false, &write_file_intrinsic },
  { STR_LIT("delete-file"), 1, false, &delete_file_intrinsic },
  // Paths
  { STR_LIT("get-current-path"), 0, true, &get_current_path_intrinsic },
  { STR_LIT("set-current-path"), 1, false, &set_current_path_intrinsic },
  { STR_LIT("get-absolute-path"), 1, true, &get_absolute_path_intrinsic },
  { STR_LIT("list-directory"), 1, true, &list_directory_intrinsic },
  // Arguments
  { STR_LIT("get-args"), 0, true, &get_args_intrinsic },
  // Sockets
  { STR_LIT("create-server"), 1, true, &create_server_intrinsic },
  { STR_LIT("create-client"), 2, true, &create_client_intrinsic },
  { STR_LIT("accept-connection"), 2, true, &accept_connection_intrinsic },
  { STR_LIT("close-connection"), 1, false, &close_connection_intrinsic },
  { STR_LIT("send"), 2, false, &send_intrinsic },
  { STR_LIT("receive"), 1, true, &receive_intrinsic },
  // Processes
  { STR_LIT("run-command"), 1, true, &run_command_intrinsic },
};

u32 system_intrinsics_len = ARRAY_LEN(system_intrinsics);
