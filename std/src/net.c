#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <netdb.h>

#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_RECEIVE_BUFFER_SIZE 64

static Dict gen_socket_dict(RcArena *rc_arena, i32 socket, bool is_non_blocking) {
  Dict dict = {0};

  Value *socket_value = rc_arena_alloc(rc_arena, sizeof(Value));
  socket_value->kind = ValueKindInt;
  socket_value->as._int = socket;
  dict_push_value_str_key(rc_arena, &dict,
                          STR_LIT("socket"),
                          socket_value);

  Value *is_non_blocking_value = rc_arena_alloc(rc_arena, sizeof(Value));
  is_non_blocking_value->kind = ValueKindBool;
  is_non_blocking_value->as._bool = is_non_blocking;
  dict_push_value_str_key(rc_arena, &dict,
                          STR_LIT("is-non-blocking"),
                          is_non_blocking_value);

  return dict;
}

bool create_server_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);

  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  fcntl(server_socket, F_SETFL, O_NONBLOCK);

  i32 enable = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable, sizeof(enable));
  setsockopt(server_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  struct sockaddr_in address = {0};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port->as._int);

  if (bind(server_socket, (struct sockaddr*) &address,
           sizeof(address)) < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  if (listen(server_socket, 3) < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  Dict server = gen_socket_dict(&vm->rc_arena, server_socket, false);
  value_stack_push_dict(&vm->stack, &vm->rc_arena, server);

  return true;
}

bool create_client_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);
  Value *server_ip_address = value_stack_pop(&vm->stack);

  char *server_ip_address_cstr = malloc(server_ip_address->as.string.len + 1);
  memcpy(server_ip_address_cstr,
         server_ip_address->as.string.ptr,
         server_ip_address->as.string.len);
  server_ip_address_cstr[server_ip_address->as.string.len] = '\0';

  char *port_cstr = malloc(port->as.string.len + 1);
  memcpy(port_cstr, port->as.string.ptr, port->as.string.len);
  port_cstr[port->as.string.len] = '\0';

  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result;

  if (getaddrinfo(server_ip_address_cstr, port_cstr, &hints, &result) < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(server_ip_address_cstr);
    return true;
  }

  i32 client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (client_socket < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  i32 connected = connect(client_socket, result->ai_addr, result->ai_addrlen);
  if (connected < 0 && errno != EAGAIN && errno != EINPROGRESS) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(server_ip_address_cstr);
    freeaddrinfo(result);
    return true;
  }

  Dict client = gen_socket_dict(&vm->rc_arena, client_socket, false);
  value_stack_push_dict(&vm->stack, &vm->rc_arena, client);
  free(server_ip_address_cstr);
  freeaddrinfo(result);

  return true;
}

bool accept_connection_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);
  Value *server = value_stack_pop(&vm->stack);

  Value *socket = dict_get_value_str_key(&vm->rc_arena,
                                         &server->as.dict,
                                         STR_LIT("socket"));
  if (socket->kind != ValueKindInt)
    PANIC("accept-connection: wrong argument kinds\n");

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port->as._int);

  socklen_t address_size = sizeof(address);
  i32 client_socket = accept(socket->as._int,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0) {
    ERROR("%s\n", strerror(errno));

    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  Dict client = gen_socket_dict(&vm->rc_arena, client_socket, false);
  value_stack_push_dict(&vm->stack, &vm->rc_arena, client);

  return true;
}

bool close_connection_intrinsic(Vm *vm) {
  Value *client = value_stack_pop(&vm->stack);

  Value *socket = dict_get_value_str_key(&vm->rc_arena,
                                         &client->as.dict,
                                         STR_LIT("socket"));
  if (socket->kind != ValueKindInt)
    PANIC("close-connection: wrong argument kind\n");

  close(socket->as._int);

  return true;
}

bool send_intrinsic(Vm *vm) {
  Value *message = value_stack_pop(&vm->stack);
  Value *receiver = value_stack_pop(&vm->stack);

  Value *socket = dict_get_value_str_key(&vm->rc_arena,
                                         &receiver->as.dict,
                                         STR_LIT("socket"));
  if (socket->kind != ValueKindInt)
    PANIC("send: wrong argument kinds\n");

  send(socket->as._int, message->as.string.ptr,
       message->as.string.len, 0);

  return true;
}

bool receive_size_intrinsic(Vm *vm) {
  Value *size = value_stack_pop(&vm->stack);
  Value *receiver = value_stack_pop(&vm->stack);

  Value *socket = dict_get_value_str_key(&vm->rc_arena,
                                         &receiver->as.dict,
                                         STR_LIT("socket"));
  if (socket->kind != ValueKindInt)
    PANIC("receive-size: wrong argument kinds\n");

  Value *is_non_blocking = dict_get_value_str_key(&vm->rc_arena,
                                                  &receiver->as.dict,
                                                  STR_LIT("is-non-blocking"));
  if (is_non_blocking->kind != ValueKindBool)
    PANIC("receive-size: wrong argument kind\n");

  Str buffer;
  buffer.ptr = rc_arena_alloc(&vm->rc_arena, size->as._int);

  if (is_non_blocking->as._bool)
    buffer.len = recv(socket->as._int, buffer.ptr, size->as._int, MSG_DONTWAIT);
  else
    buffer.len = recv(socket->as._int, buffer.ptr, size->as._int, 0);

  if ((i32) buffer.len < 0) {
    if (is_non_blocking->as._bool) {
      if (errno != EAGAIN && errno != EINPROGRESS) {
        ERROR("%s\n", strerror(errno));

        value_stack_push_unit(&vm->stack, &vm->rc_arena);
        return true;
      }
    } else {
      ERROR("%s\n", strerror(errno));

      value_stack_push_unit(&vm->stack, &vm->rc_arena);
      return true;
    }
  }

  if (buffer.len == 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    rc_arena_free(&vm->rc_arena, buffer.ptr);

    return true;
  }

  value_stack_push_string(&vm->stack, &vm->rc_arena, buffer);

  return true;
}

bool receive_intrinsic(Vm *vm) {
  Value *receiver = value_stack_pop(&vm->stack);

  Value *socket = dict_get_value_str_key(&vm->rc_arena,
                                         &receiver->as.dict,
                                         STR_LIT("socket"));
  if (socket->kind != ValueKindInt)
    PANIC("receive: wrong argument kind\n");

  Value *is_non_blocking = dict_get_value_str_key(&vm->rc_arena,
                                                  &receiver->as.dict,
                                                  STR_LIT("is-non-blocking"));
  if (is_non_blocking->kind != ValueKindBool)
    PANIC("receive: wrong argument kind\n");

  u32 cap = DEFAULT_RECEIVE_BUFFER_SIZE;

  Str buffer = {0};
  buffer.ptr = rc_arena_alloc(&vm->rc_arena, cap);

  i32 len = 0;
  while (true) {
    len = recv(socket->as._int,
               buffer.ptr + buffer.len,
               cap - buffer.len, MSG_DONTWAIT);

    if (len == 0 && is_non_blocking->as._bool)
      break;

    if (len < 0) {
      if (is_non_blocking->as._bool) {
        if (errno != EAGAIN && errno != EINPROGRESS) {
          ERROR("%s\n", strerror(errno));

          value_stack_push_unit(&vm->stack, &vm->rc_arena);
          return true;
        }
      } else {
        ERROR("%s\n", strerror(errno));

        value_stack_push_unit(&vm->stack, &vm->rc_arena);
        return true;
      }

      break;
    }

    buffer.len += (u32) len;

    if (buffer.len >= cap) {
      char *prev_ptr = buffer.ptr;

      cap += DEFAULT_RECEIVE_BUFFER_SIZE;
      buffer.ptr = rc_arena_alloc(&vm->rc_arena, cap);
      memcpy(buffer.ptr, prev_ptr, buffer.len);

      rc_arena_free(&vm->rc_arena, prev_ptr);
    }
  }

  if (buffer.len == 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    rc_arena_free(&vm->rc_arena, buffer.ptr);

    return true;
  }

  value_stack_push_string(&vm->stack, &vm->rc_arena, buffer);

  return true;
}

Intrinsic net_intrinsics[] = {
  { STR_LIT("create-server"), true, 1, { ValueKindInt }, &create_server_intrinsic },
  { STR_LIT("create-client"), true, 2,
    { ValueKindString, ValueKindString },
    &create_client_intrinsic },
  { STR_LIT("accept-connection"), true, 2,
    { ValueKindDict, ValueKindInt },
    &accept_connection_intrinsic },
  { STR_LIT("close-connection"), false, 1, { ValueKindDict }, &close_connection_intrinsic },
  { STR_LIT("send"), false, 2, { ValueKindDict, ValueKindString }, &send_intrinsic },
  { STR_LIT("receive-size"), true, 2,
    { ValueKindDict, ValueKindInt },
    &receive_size_intrinsic },
  { STR_LIT("receive"), true, 1, { ValueKindDict }, &receive_intrinsic },
};

u32 net_intrinsics_len = ARRAY_LEN(net_intrinsics);
