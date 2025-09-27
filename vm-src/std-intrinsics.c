#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "std-intrinsics.h"
#include "io.h"
#include "shl_log.h"
#include "shl_arena.h"

#define DEFAULT_INPUT_BUFFER_SIZE   64
#define DEFAULT_RECEIVE_BUFFER_SIZE 64

void exit_intrinsic(Vm *vm) {
  Value exit_code = value_stack_pop(&vm->stack);
  if (exit_code.kind != ValueKindNumber) {
    ERROR("exit: wrong argument kind\n");
    exit(1);
  }

  exit(1);
}

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

      print_value(stack, &node->value, level);

      node = node->next;
    }

    fputc(']', stdout);
  } break;

  case ValueKindString: {
    fputc('\'', stdout);
    str_print(value->as.string);
    fputc('\'', stdout);
  } break;

  case ValueKindNumber: {
    printf("%ld", value->as.number);
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
    fputs("{\n", stdout);

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
    fputc('}', stdout);
  } break;
  }
}

void print_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  print_value(&vm->stack, &value, 0);
}

void println_intrinsic(Vm *vm) {
  print_intrinsic(vm);
  fputc('\n', stdout);
}

void input_intrinsic(Vm *vm) {
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
}

void read_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString) {
    ERROR("read-file: wrong argument kind\n");
    exit(1);
  }

  char *path_cstring = aalloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  Str content = read_file(path_cstring);
  if (content.len == (u32) -1)
    value_stack_push_unit(&vm->stack);
  else
    value_stack_push_string(&vm->stack, content);
}

void write_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  Value content = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindString ||
      content.kind != ValueKindString) {
    ERROR("write-file: wrong argument kinds\n");
    exit(1);
  }

  char *path_cstring = aalloc(path.as.string.len + 1);
  memcpy(path_cstring, path.as.string.ptr, path.as.string.len);
  path_cstring[path.as.string.len] = '\0';

  write_file(path_cstring, content.as.string);
}

void get_args_intrinsic(Vm *vm) {
  value_stack_push_list(&vm->stack, vm->args);
}

void create_server_intrinsic(Vm *vm) {
  Value port = value_stack_pop(&vm->stack);
  if (port.kind != ValueKindNumber) {
    ERROR("create-server: wrong argument kind\n");
    exit(1);
  }

  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port.as.number);

  if (bind(server_socket, (struct sockaddr*) &address,
           sizeof(address)) < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  if (listen(server_socket, 3) < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  value_stack_push_number(&vm->stack, server_socket);
}

void create_client_intrinsic(Vm *vm) {
  Value port = value_stack_pop(&vm->stack);
  Value server_ip_address = value_stack_pop(&vm->stack);
  if (server_ip_address.kind != ValueKindString ||
      port.kind != ValueKindNumber) {
    ERROR("create-client: wrong argument kind\n");
    exit(1);
  }

  i32 client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port.as.number);

  char *server_ip_address_cstr = malloc(server_ip_address.as.string.len + 1);
  memcpy(server_ip_address_cstr,
         server_ip_address.as.string.ptr,
         server_ip_address.as.string.len);
  server_ip_address_cstr[server_ip_address.as.string.len] = '\0';

  if (inet_pton(AF_INET,
                server_ip_address_cstr,
                &server_address.sin_addr) < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  if (connect(client_socket,
              (struct sockaddr*) &server_address,
              sizeof(server_address)) < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  value_stack_push_number(&vm->stack, client_socket);
}

void accept_connection_intrinsic(Vm *vm) {
  Value port = value_stack_pop(&vm->stack);
  Value server_socket = value_stack_pop(&vm->stack);
  if (server_socket.kind != ValueKindNumber ||
      port.kind != ValueKindNumber) {
    ERROR("accept-connection: wrong argument kind\n");
    exit(1);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port.as.number);

  u32 address_size;
  i32 client_socket = accept(server_socket.as.number,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  value_stack_push_number(&vm->stack, client_socket);
}

void close_connection_intrinsic(Vm *vm) {
  Value client_socket = value_stack_pop(&vm->stack);
  if (client_socket.kind != ValueKindNumber) {
    ERROR("close-connection: wrong argument kind\n");
    exit(1);
  }

  close(client_socket.as.number);
}

void send_intrinsic(Vm *vm) {
  Value message = value_stack_pop(&vm->stack);
  Value receiver = value_stack_pop(&vm->stack);
  if (receiver.kind != ValueKindNumber ||
      message.kind != ValueKindString) {
    ERROR("send: wrong argument kind\n");
    exit(1);
  }

  send(receiver.as.number, message.as.string.ptr, message.as.string.len, 0);
}

void receive_intrinsic(Vm *vm) {
  Value receiver = value_stack_pop(&vm->stack);
  if (receiver.kind != ValueKindNumber) {
    ERROR("receive: wrong argument kind\n");
    exit(1);
  }

  Str buffer;
  buffer.len = DEFAULT_RECEIVE_BUFFER_SIZE;
  buffer.ptr = rc_arena_alloc(vm->rc_arena, buffer.len);

  u32 len;
  while ((len = read(receiver.as.number,
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
}

void head_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("head: wrong argument kind\n");
    exit(1);
  }

  if (!value.as.list->next) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  DA_APPEND(vm->stack, value.as.list->next->value);
}

void tail_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("tail: wrong argument kind\n");
    exit(1);
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  if (value.as.list->next)
    new_list->next = list_clone(vm->rc_arena, value.as.list->next->next);

  value_stack_push_list(&vm->stack, new_list);
}

void last_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("last: wrong argument kind\n");
    exit(1);
  }

  if (!value.as.list->next) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  ListNode *node = value.as.list->next;
  while (node && node->next)
    node = node->next;

  DA_APPEND(vm->stack, node->value);
}

void nth_intrinsic(Vm *vm) {
  Value index = value_stack_pop(&vm->stack);
  Value list = value_stack_pop(&vm->stack);
  if (list.kind != ValueKindList ||
      index.kind != ValueKindNumber) {
    ERROR("nth: wrong argument kinds\n");
    exit(1);
  }

  ListNode *node = list.as.list->next;
  ListNode *prev_node = list.as.list->next;
  u32 i = 0;
  while (node && i < index.as.number) {
    node = node->next;
    if (prev_node->next)
      prev_node = prev_node->next;
    ++i;
  }

  if (i < index.as.number)
    value_stack_push_unit(&vm->stack);
  else
    DA_APPEND(vm->stack, prev_node->value);
}

void len_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind == ValueKindList) {
    ListNode *node = value.as.list->next;
    u32 len = 0;
    while (node) {
      node = node->next;
      ++len;
    }

    value_stack_push_number(&vm->stack, len);
  } else if (value.kind == ValueKindString) {
    value_stack_push_number(&vm->stack, value.as.string.len);
  } else {
    ERROR("len: wrong argument kind\n");
    exit(1);
  }
}

void is_empty_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList &&
      value.kind != ValueKindString) {
    ERROR("is-empty: wrong argument kind\n");
    exit(1);
  }

  if (value.kind == ValueKindList)
    value_stack_push_bool(&vm->stack, value.as.list->next == NULL);
  else
    value_stack_push_bool(&vm->stack, value.as.string.len == 0);
}

void get_range_intrinsic(Vm *vm) {
  Value end = value_stack_pop(&vm->stack);
  Value begin = value_stack_pop(&vm->stack);
  Value value = value_stack_pop(&vm->stack);
  if (begin.kind != ValueKindNumber ||
      end.kind != ValueKindNumber ||
      (value.kind != ValueKindList &&
       value.kind != ValueKindString)) {
    ERROR("get-range: wrong argument kinds\n");
    exit(1);
  }

  DA_APPEND(vm->stack, value);
  len_intrinsic(vm);
  Value len = value_stack_pop(&vm->stack);

  if (begin.as.number < 0 || begin.as.number >= len.as.number ||
      end.as.number <= 0 || end.as.number >= len.as.number ||
      begin.as.number >= end.as.number) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  if (value.kind == ValueKindList) {
    ListNode *node = value.as.list->next;
    ListNode *sub_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < begin.as.number; ++i)
      node = node->next;

    for (u32 i = 0; i < end.as.number - begin.as.number; ++i) {
      sub_list_node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    value_stack_push_list(&vm->stack, sub_list);
  } else {
    Str sub_string = {
      value.as.string.ptr + begin.as.number,
      value.as.string.len - begin.as.number - end.as.number,
    };

    value_stack_push_string(&vm->stack, sub_string);
  }
}

void map_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList) {
    ERROR("map: wrong argument kinds\n");
    exit(1);
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list.as.list->next;
  while (node) {
    ValueStack args = {0};
    DA_APPEND(args, node->value);
    execute_func(vm, func.as.func.name, &args, true);

    *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    (*new_list_next)->value = value_stack_pop(&vm->stack);
    new_list_next = &(*new_list_next)->next;

    free(args.items);

    node = node->next;
  }

  value_stack_push_list(&vm->stack, new_list);
}

void filter_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList) {
    ERROR("filter: wrong argument kinds\n");
    exit(1);
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list.as.list->next;
  while (node) {
    ValueStack args = {0};
    DA_APPEND(args, node->value);
    execute_func(vm, func.as.func.name, &args, true);

    Value is_ok = value_stack_pop(&vm->stack);
    if (is_ok.kind != ValueKindBool) {
      ERROR("filter: wrong argument kinds\n");
      exit(1);
    }

    if (is_ok.as._bool) {
      *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      (*new_list_next)->value = node->value;
      new_list_next = &(*new_list_next)->next;
    }

    free(args.items);

    node = node->next;
  }

  value_stack_push_list(&vm->stack, new_list);
}

void reduce_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value initial_value = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList) {
    ERROR("reduce: wrong argument kinds\n");
    exit(1);
  }

  Value accumulator = initial_value;
  ListNode *node = list.as.list->next;
  while (node) {
    ValueStack args = {0};
    DA_APPEND(args, accumulator);
    DA_APPEND(args, node->value);
    execute_func(vm, func.as.func.name, &args, true);

    accumulator = value_stack_pop(&vm->stack);
    if (accumulator.kind != initial_value.kind) {
      ERROR("reduce: return value's and accumulator's types should be equal\n");
      exit(1);
    }

    node = node->next;
  }

  DA_APPEND(vm->stack, accumulator);
}

void split_intrinsic(Vm *vm) {
  Value delimeter = value_stack_pop(&vm->stack);
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString ||
      delimeter.kind != ValueKindString) {
    ERROR("split: wrong argument kinds\n");
    exit(1);
  }

  ListNode *list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode *node = list;
  u32 index = 0, i = 0;
  for (; i < string.as.string.len; ++i) {
    u32 found = true;
    for (u32 j = 0; j + i < string.as.string.len && j < delimeter.as.string.len; ++j) {
      if (string.as.string.ptr[j + i] != delimeter.as.string.ptr[j]) {
        found = false;
        break;
      }
    }

    if (found) {
      node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      node->next->value = (Value) {
        ValueKindString,
        { .string = STR(string.as.string.ptr + index, i - index) },
      };

      index = i + 1;
      node = node->next;
    }
  }

  if (i > 0) {
    node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    node->next->value = (Value) {
      ValueKindString,
      { .string = STR(string.as.string.ptr + index, i - index) },
    };
  }

  value_stack_push_list(&vm->stack, list);
}

void eat_str_intrinsic(Vm *vm) {
  Value pattern = value_stack_pop(&vm->stack);
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString ||
      pattern.kind != ValueKindString) {
    ERROR("eat-str: wrong argument kinds\n");
    exit(1);
  }

  if (string.as.string.len < pattern.as.string.len) {
    value_stack_push_bool(&vm->stack, false);
    return;
  }

  Str string_begin = {
    string.as.string.ptr,
    pattern.as.string.len,
  };

  bool matches = str_eq(string_begin, pattern.as.string);

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));

  new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  new_list->next->value = (Value) { ValueKindBool, { ._bool = matches } };

  new_list->next->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  Str new_string;
  new_string.len = string.as.string.len - pattern.as.string.len;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, string.as.string.ptr + pattern.as.string.len, new_string.len);
  new_list->next->next->value = (Value) { ValueKindString, { .string = new_string } };

  value_stack_push_list(&vm->stack, new_list);
}

static void eat_byte(Vm *vm, char *intrinsic_name, u32 size) {
  Value string = value_stack_pop(&vm->stack);
  if (string.kind != ValueKindString) {
    ERROR("%s: wrong argument kinds\n", intrinsic_name);
    exit(1);
  }

  if (string.as.string.len < size) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  i64 number = 0;
  switch (size) {
  case 8: number = *(i64 *) string.as.string.ptr; break;
  case 4: number = *(i32 *) string.as.string.ptr; break;
  case 2: number = *(i16 *) string.as.string.ptr; break;
  case 1: number = *(i8 *) string.as.string.ptr; break;
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));

  new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  new_list->next->value = (Value) { ValueKindNumber, { .number = number } };

  new_list->next->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  Str new_string;
  new_string.len = string.as.string.len - size;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, string.as.string.ptr + size, new_string.len);
  new_list->next->next->value = (Value) { ValueKindString, { .string = new_string } };

  value_stack_push_list(&vm->stack, new_list);
}

void eat_byte_64_intrinsic(Vm *vm) {
  eat_byte(vm, "eat-byte-64", 8);
}

void eat_byte_32_intrinsic(Vm *vm) {
  eat_byte(vm, "eat-byte-32", 4);
}

void eat_byte_16_intrinsic(Vm *vm) {
  eat_byte(vm, "eat-byte-16", 2);
}

void eat_byte_8_intrinsic(Vm *vm) {
  eat_byte(vm, "eat-byte-8", 1);
}

void str_to_num_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindString) {
    ERROR("str-to-num: wrong argument kind\n");
    exit(1);
  }

  value_stack_push_number(&vm->stack, str_to_i64(value.as.string));
}

void num_to_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindNumber) {
    ERROR("num-to-str: wrong argument kind\n");
    exit(1);
  }

  StringBuilder sb = {0};
  sb_push_i64(&sb, value.as.number);

  Str new_string;
  new_string.len = sb.len;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
  memcpy(new_string.ptr, sb.buffer, sb.len);
  free(sb.buffer);

  value_stack_push_string(&vm->stack, new_string);
}

void bool_to_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("bool-to-str: wrong argument kind\n");
    exit(1);
  }

  Str string;
  char *cstring;

  if (value.as._bool) {
    string.len = 4;
    cstring = "true";
  } else {
    string.len = 5;
    cstring = "false";
  }

  string.ptr = rc_arena_alloc(vm->rc_arena, string.len);
  memcpy(string.ptr, cstring, string.len);

  value_stack_push_string(&vm->stack, string);
}

void bool_to_num_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("bool-to-num: wrong argument kind\n");
    exit(1);
  }

  value_stack_push_number(&vm->stack, value.as._bool);
}

static void byte_to_str(Vm *vm, char *intrinsic_name, u32 size) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindNumber) {
    ERROR("%s: wrong argument kind\n", intrinsic_name);
    exit(1);
  }

  Str new_string;
  new_string.len = size;
  new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);

  switch (size) {
  case 8: *(i64 *) new_string.ptr = value.as.number; break;
  case 4: *(i32 *) new_string.ptr = value.as.number; break;
  case 2: *(i16 *) new_string.ptr = value.as.number; break;
  case 1: *(i8 *) new_string.ptr = value.as.number; break;
  }

  value_stack_push_string(&vm->stack, new_string);
}

void byte_64_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-64-to-str", 8);
}

void byte_32_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-32-to-str", 4);
}

void byte_16_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-16-to-str", 2);
}

void byte_8_to_str_intrinsic(Vm *vm) {
  byte_to_str(vm, "byte-8-to-str", 1);
}

void add_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_number(&vm->stack, a.as.number + b.as.number);
  } else if (a.kind == ValueKindString &&
             b.kind == ValueKindString) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a.as.string);
    sb_push_str(&sb, b.as.string);

    Str new_string;
    new_string.len = sb.len;
    new_string.ptr = rc_arena_alloc(vm->rc_arena, new_string.len);
    memcpy(new_string.ptr, sb.buffer, new_string.len);
    free(sb.buffer);

    value_stack_push_string(&vm->stack, new_string);
  } else if (a.kind == ValueKindList &&
             b.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a.as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node)
      node->next = list_clone(vm->rc_arena, b.as.list->next);

    value_stack_push_list(&vm->stack, new_list);
  } else if (a.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a.as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node) {
      node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      node->next->value = b;
    }

    value_stack_push_list(&vm->stack, new_list);
  } else if (b.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next->value = a;
    new_list->next->next = list_clone(vm->rc_arena, b.as.list->next);

    value_stack_push_list(&vm->stack, new_list);
  } else {
    ERROR("add: wrong argument kinds\n");
    exit(1);
  }
}

static void prepare_two_numbers(Value *a, Value *b, char *intrinsic_name, Vm *vm) {
  *b = value_stack_pop(&vm->stack);
  *a = value_stack_pop(&vm->stack);

  if (a->kind != ValueKindNumber ||
      b->kind != ValueKindNumber) {
    ERROR("%s: wrong argument kinds\n", intrinsic_name);
    exit(1);
  }
}

void sub_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "sub", vm);

  value_stack_push_number(&vm->stack, a.as.number - b.as.number);
}

void mul_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mul", vm);

  value_stack_push_number(&vm->stack, a.as.number * b.as.number);
}

void div_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "div", vm);

  value_stack_push_number(&vm->stack, a.as.number / b.as.number);
}

void mod_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mod", vm);

  value_stack_push_number(&vm->stack, a.as.number % b.as.number);
}

void abs_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindNumber) {
    ERROR("abs: wrong argument kind\n");
    exit(1);
  }

  if (value.as.number < 0)
    value.as.number = -value.as.number;

  DA_APPEND(vm->stack, value);
}

void min_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "min", vm);

  value_stack_push_number(&vm->stack, a.as.number <= b.as.number ?
                                      a.as.number :
                                      b.as.number);
}

void max_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "max", vm);

  value_stack_push_number(&vm->stack, a.as.number >= b.as.number ?
                                      a.as.number :
                                      b.as.number);
}

void eq_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number == b.as.number);
  } else if (a.kind == ValueKindString &&
             b.kind == ValueKindString) {
    value_stack_push_bool(&vm->stack, str_eq(a.as.string, b.as.string));
  } else {
    ERROR("eq: wrong argument kinds: %u:%u\n", a.kind, b.kind);
    exit(1);
  }
}

void ne_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number != b.as.number);
  } else if (a.kind == ValueKindString &&
             b.kind == ValueKindString) {
    value_stack_push_bool(&vm->stack, !str_eq(a.as.string, b.as.string));
  } else {
    ERROR("ne: wrong argument kinds\n");
    exit(1);
  }
}

void ls_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ls", vm);

  value_stack_push_bool(&vm->stack, a.as.number < b.as.number);
}

void le_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "le", vm);

  value_stack_push_bool(&vm->stack, a.as.number <= b.as.number);
}

void gt_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm);

  value_stack_push_bool(&vm->stack, a.as.number > b.as.number);
}

void ge_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ge", vm);

  value_stack_push_bool(&vm->stack, a.as.number >= b.as.number);
}

void and_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number & b.as.number);
  } else if (a.kind == ValueKindBool &&
             b.kind == ValueKindBool) {
    value_stack_push_bool(&vm->stack, a.as._bool & b.as._bool);
  } else {
    ERROR("and: wrong argument kinds\n");
    exit(1);
  }
}

void or_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number | b.as.number);
  } else if (a.kind == ValueKindBool &&
             b.kind == ValueKindBool) {
    value_stack_push_bool(&vm->stack, a.as._bool | b.as._bool);
  } else {
    ERROR("or: wrong argument kinds\n");
    exit(1);
  }
}

void xor_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number ^ b.as.number);
  } else if (a.kind == ValueKindBool &&
             b.kind == ValueKindBool) {
    value_stack_push_bool(&vm->stack, a.as._bool ^ b.as._bool);
  } else {
    ERROR("xor: wrong argument kinds\n");
    exit(1);
  }
}

void not_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("not: wrong argument kind\n");
    exit(1);
  }

  value_stack_push_bool(&vm->stack, !value.as._bool);
}

void is_unit_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindUnit);
}

void is_list_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindList);
}

void is_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindString);
}

void is_number_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindNumber);
}

void is_bool_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindBool);
}

void is_fun_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindFunc);
}

void is_record_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindRecord);
}

void type_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);

  switch (value.kind) {
  case ValueKindUnit: {
    value_stack_push_string(&vm->stack, STR_LIT("unit"));
  } break;

  case ValueKindList: {
    value_stack_push_string(&vm->stack, STR_LIT("list"));
  } break;

  case ValueKindString: {
    value_stack_push_string(&vm->stack, STR_LIT("str"));
  } break;

  case ValueKindNumber: {
    value_stack_push_string(&vm->stack, STR_LIT("number"));
  } break;

  case ValueKindBool: {
    value_stack_push_string(&vm->stack, STR_LIT("bool"));
  } break;

  case ValueKindFunc: {
    value_stack_push_string(&vm->stack, STR_LIT("fun"));
  } break;

  case ValueKindRecord: {
    value_stack_push_string(&vm->stack, STR_LIT("record"));
  } break;

  default: {
    ERROR("Unknown type\n");
    exit(1);
  }
  }
}

Intrinsic std_intrinsics[] = {
  // System
  { STR_LIT("exit"), 1, false, &exit_intrinsic },
  // Io
  { STR_LIT("print"), 1, false, &print_intrinsic },
  { STR_LIT("println"), 1, false, &println_intrinsic },
  { STR_LIT("input"), 0, true, &input_intrinsic },
  { STR_LIT("read-file"), 1, true, &read_file_intrinsic },
  { STR_LIT("write-file"), 2, false, &write_file_intrinsic },
  { STR_LIT("get-args"), 0, true, &get_args_intrinsic },
  { STR_LIT("create-server"), 1, true, &create_server_intrinsic },
  { STR_LIT("create-client"), 2, true, &create_client_intrinsic },
  { STR_LIT("accept-connection"), 2, true, &accept_connection_intrinsic },
  { STR_LIT("close-connection"), 1, false, &close_connection_intrinsic },
  { STR_LIT("send"), 2, false, &send_intrinsic },
  { STR_LIT("receive"), 1, true, &receive_intrinsic },
  // Base
  { STR_LIT("head"), 1, true, &head_intrinsic },
  { STR_LIT("tail"), 1, true, &tail_intrinsic },
  { STR_LIT("last"), 1, true, &last_intrinsic },
  { STR_LIT("nth"), 2, true, &nth_intrinsic },
  { STR_LIT("len"), 1, true, &len_intrinsic },
  { STR_LIT("is-empty"), 1, true, &is_empty_intrinsic },
  { STR_LIT("get-range"), 3, true, &get_range_intrinsic },
  // Functional stuff
  { STR_LIT("map"), 2, true, &map_intrinsic },
  { STR_LIT("filter"), 2, true, &filter_intrinsic },
  { STR_LIT("reduce"), 3, true, &reduce_intrinsic },
  // String operations
  { STR_LIT("split"), 2, true, &split_intrinsic },
  { STR_LIT("eat-str"), 2, true, &eat_str_intrinsic },
  { STR_LIT("eat-byte-64"), 1, true, &eat_byte_64_intrinsic },
  { STR_LIT("eat-byte-32"), 1, true, &eat_byte_32_intrinsic },
  { STR_LIT("eat-byte-16"), 1, true, &eat_byte_16_intrinsic },
  { STR_LIT("eat-byte-8"), 1, true, &eat_byte_8_intrinsic },
  // Conversions
  { STR_LIT("str-to-num"), 1, true, &str_to_num_intrinsic },
  { STR_LIT("num-to-str"), 1, true, &num_to_str_intrinsic },
  { STR_LIT("bool-to-str"), 1, true, &bool_to_str_intrinsic },
  { STR_LIT("bool-to-num"), 1, true, &bool_to_num_intrinsic },
  { STR_LIT("byte-64-to-str"), 1, true, &byte_64_to_str_intrinsic },
  { STR_LIT("byte-32-to-str"), 1, true, &byte_32_to_str_intrinsic },
  { STR_LIT("byte-16-to-str"), 1, true, &byte_16_to_str_intrinsic },
  { STR_LIT("byte-8-to-str"), 1, true, &byte_8_to_str_intrinsic },
  // Math
  { STR_LIT("+"), 2, true, &add_intrinsic },
  { STR_LIT("-"), 2, true, &sub_intrinsic },
  { STR_LIT("*"), 2, true, &mul_intrinsic },
  { STR_LIT("/"), 2, true, &div_intrinsic },
  { STR_LIT("%"), 2, true, &mod_intrinsic },
  // Advanced math
  { STR_LIT("abs"), 1, true, &abs_intrinsic },
  { STR_LIT("min"), 2, true, &min_intrinsic },
  { STR_LIT("max"), 2, true, &max_intrinsic },
  // TODO: floating point numbers
  // { STR_LIT("sqrt"), 1, true, &sqrt_intrinsic },
  // { STR_LIT("pow"), 2, true, &pow_intrinsic },
  // { STR_LIT("round"), 1, true, &round_intrinsic },
  // Comparisons
  { STR_LIT("=="), 2, true, &eq_intrinsic },
  { STR_LIT("!="), 2, true, &ne_intrinsic },
  { STR_LIT("<"), 2, true, &ls_intrinsic },
  { STR_LIT("<="), 2, true, &le_intrinsic },
  { STR_LIT(">"), 2, true, &gt_intrinsic },
  { STR_LIT(">="), 2, true, &ge_intrinsic },
  // Boolean
  { STR_LIT("&"), 2, true, &and_intrinsic },
  { STR_LIT("|"), 2, true, &or_intrinsic },
  { STR_LIT("^"), 2, true, &xor_intrinsic },
  { STR_LIT("!"), 1, true, &not_intrinsic },
  // Types
  { STR_LIT("unit?"), 1, true, &is_unit_intrinsic },
  { STR_LIT("list?"), 1, true, &is_list_intrinsic },
  { STR_LIT("str?"), 1, true, &is_str_intrinsic },
  { STR_LIT("number?"), 1, true, &is_number_intrinsic },
  { STR_LIT("bool?"), 1, true, &is_bool_intrinsic },
  { STR_LIT("fun?"), 1, true, &is_fun_intrinsic },
  { STR_LIT("record?"), 1, true, &is_record_intrinsic },
  { STR_LIT("type"), 1, true, &type_intrinsic },
};

u32 std_intrinsics_len = ARRAY_LEN(std_intrinsics);
