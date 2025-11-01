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
#define POLL_TIMEOUT_MS             10

bool create_server_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);

  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
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
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    close(server_socket);
    return true;
  }

  if (listen(server_socket, 3) < 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    close(server_socket);
    return true;
  }

  value_stack_push_int(&vm->stack, &vm->rc_arena, server_socket);

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
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(server_ip_address_cstr);
    free(port_cstr);
    return true;
  }

  i32 client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(server_ip_address_cstr);
    free(port_cstr);
    return true;
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  i32 connected = connect(client_socket, result->ai_addr, result->ai_addrlen);
  if (connected < 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    free(server_ip_address_cstr);
    free(port_cstr);
    freeaddrinfo(result);
    return true;
  }

  value_stack_push_int(&vm->stack, &vm->rc_arena, client_socket);
  free(server_ip_address_cstr);
  freeaddrinfo(result);

  return true;
}

bool accept_connection_intrinsic(Vm *vm) {
  Value *port = value_stack_pop(&vm->stack);
  Value *server = value_stack_pop(&vm->stack);

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port->as._int);

  socklen_t address_size = sizeof(address);
  i32 client_socket = accept(server->as._int,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0) {
    value_stack_push_unit(&vm->stack, &vm->rc_arena);
    return true;
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  value_stack_push_int(&vm->stack, &vm->rc_arena, client_socket);

  return true;
}

bool close_connection_intrinsic(Vm *vm) {
  Value *client = value_stack_pop(&vm->stack);

  close(client->as._int);

  return true;
}

bool send_intrinsic(Vm *vm) {
  Value *message = value_stack_pop(&vm->stack);
  Value *receiver = value_stack_pop(&vm->stack);

  send(receiver->as._int, message->as.string.ptr,
       message->as.string.len, 0);

  return true;
}

bool receive_size_intrinsic(Vm *vm) {
  Value *size = value_stack_pop(&vm->stack);
  Value *receiver = value_stack_pop(&vm->stack);

  Str buffer = { rc_arena_alloc(&vm->rc_arena, size->as._int), 0 };

  struct pollfd pfd;
  pfd.fd = receiver->as._int;
  pfd.events = POLLIN | POLLERR | POLLHUP;
  poll(&pfd, 1, POLL_TIMEOUT_MS);

  if (pfd.revents != 0)
    buffer.len = recv(receiver->as._int, buffer.ptr, size->as._int, 0);

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

  u32 cap = DEFAULT_RECEIVE_BUFFER_SIZE;
  Str buffer = { rc_arena_alloc(&vm->rc_arena, cap), 0 };

  struct pollfd pfd;
  pfd.fd = receiver->as._int;
  pfd.events = POLLIN | POLLERR | POLLHUP;
  i32 len = 0;

  while (true) {
    poll(&pfd, 1, POLL_TIMEOUT_MS);
    if (pfd.revents == 0)
      break;

    len = recv(receiver->as._int,
               buffer.ptr + buffer.len,
               cap - buffer.len, 0);

    if (len == 0)
      break;

    if (len < 0) {
      value_stack_push_unit(&vm->stack, &vm->rc_arena);
      return true;
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
