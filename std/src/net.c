#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include "aether/vm.h"

#define DEFAULT_RECEIVE_BUFFER_SIZE 64

bool create_server_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);

  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    value_stack_push_unit(&vm->stack);
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
    value_stack_push_unit(&vm->stack);
    return true;
  }

  if (listen(server_socket, 3) < 0) {
    value_stack_push_unit(&vm->stack);
    return true;
  }

  value_stack_push_int(&vm->stack, vm->rc_arena, server_socket);

  return true;
}

bool create_client_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);
  Value *server_ip_address = value_stack_pop(&vm->stack);

  i32 client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack);

    return true;
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port->as._int);

  char *server_ip_address_cstr = malloc(server_ip_address->as.string.str.len + 1);
  memcpy(server_ip_address_cstr,
         server_ip_address->as.string.str.ptr,
         server_ip_address->as.string.str.len);
  server_ip_address_cstr[server_ip_address->as.string.str.len] = '\0';

  if (inet_pton(AF_INET,
                server_ip_address_cstr,
                &server_address.sin_addr) < 0) {
    value_stack_push_unit(&vm->stack);

    free(server_ip_address_cstr);
    return true;
  }

  i32 connected = connect(client_socket,
                          (struct sockaddr*) &server_address,
                          sizeof(server_address));
  if (connected < 0 && errno != EINPROGRESS) {
    value_stack_push_unit(&vm->stack);

    free(server_ip_address_cstr);
    return true;
  }

  value_stack_push_int(&vm->stack, vm->rc_arena, client_socket);

  free(server_ip_address_cstr);
  return true;
}

bool accept_connection_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);
  Value *server_socket = value_stack_pop(&vm->stack);

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port->as._int);

  socklen_t address_size = sizeof(address);
  i32 client_socket = accept(server_socket->as._int,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack);

    return true;
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  value_stack_push_int(&vm->stack, vm->rc_arena, client_socket);

  return true;
}

bool close_connection_intrinsic(Vm *vm) {
  Value *client_socket = value_stack_pop(&vm->stack);

  close(client_socket->as._int);

  return true;
}

bool send_intrinsic(Vm *vm) {
  Value *message = value_stack_pop(&vm->stack);
  Value *receiver = value_stack_pop(&vm->stack);

  send(receiver->as._int, message->as.string.str.ptr,
       message->as.string.str.len, 0);

  return true;
}

bool receive_size_intrinsic(Vm *vm) {
  Value *size = value_stack_pop(&vm->stack);
  Value *receiver = value_stack_pop(&vm->stack);

  Str buffer;
  buffer.len = size->as._int;
  buffer.ptr = rc_arena_alloc(vm->rc_arena, buffer.len);
  recv(receiver->as._int, buffer.ptr, buffer.len, 0);

  value_stack_push_string(&vm->stack, vm->rc_arena, buffer);

  return true;
}

bool receive_intrinsic(Vm *vm) {
  Value *receiver = value_stack_pop(&vm->stack);

  u32 cap = DEFAULT_RECEIVE_BUFFER_SIZE;

  Str buffer = {0};
  buffer.ptr = rc_arena_alloc(vm->rc_arena, cap);

  i32 len = 0;
  while ((len = recv(receiver->as._int,
                     buffer.ptr + buffer.len,
                     cap - buffer.len, MSG_DONTWAIT)) > 0) {
    buffer.len += (u32) len;

    if (buffer.len >= cap) {
      char *prev_ptr = buffer.ptr;

      cap += DEFAULT_RECEIVE_BUFFER_SIZE;
      buffer.ptr = rc_arena_alloc(vm->rc_arena, cap);

      memcpy(buffer.ptr, prev_ptr, buffer.len);
      rc_arena_free(vm->rc_arena, prev_ptr);
    }
  }

  if (buffer.len == 0) {
    rc_arena_free(vm->rc_arena, buffer.ptr);
    value_stack_push_unit(&vm->stack);

    return true;
  }

  value_stack_push_string(&vm->stack, vm->rc_arena, buffer);

  return true;
}

Intrinsic net_intrinsics[] = {
  { STR_LIT("create-server"), true, 1, { ValueKindInt }, &create_server_intrinsic },
  { STR_LIT("create-client"), true, 2,
    { ValueKindString, ValueKindInt },
    &create_client_intrinsic },
  { STR_LIT("accept-connection"), true, 2,
    { ValueKindInt, ValueKindInt },
    &accept_connection_intrinsic },
  { STR_LIT("close-connection"), false, 1, { ValueKindInt }, &close_connection_intrinsic },
  { STR_LIT("send"), false, 2, { ValueKindInt, ValueKindString }, &send_intrinsic },
  { STR_LIT("receive-size"), true, 2,
    { ValueKindInt, ValueKindInt },
    &receive_size_intrinsic },
  { STR_LIT("receive"), true, 1, { ValueKindInt }, &receive_intrinsic },
};

u32 net_intrinsics_len = ARRAY_LEN(net_intrinsics);
