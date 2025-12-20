#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <netdb.h>

#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_RECEIVE_BUFFER_SIZE 64
#define POLL_TIMEOUT_MS             10

Value *create_server_intrinsic(Vm *vm, Value **args) {
  Value *port = args[0];

  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
    return value_unit(vm->current_frame);

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
    close(server_socket);

    return value_unit(vm->current_frame);
  }

  if (listen(server_socket, 3) < 0) {
    close(server_socket);

    return value_unit(vm->current_frame);
  }

  return value_int(server_socket, vm->current_frame);
}

Value *create_client_intrinsic(Vm *vm, Value **args) {
  Value *server_ip_address = args[0];
  Value *port = args[1];

  char *server_ip_address_cstr = malloc(server_ip_address->as.string.str.len + 1);
  memcpy(server_ip_address_cstr,
         server_ip_address->as.string.str.ptr,
         server_ip_address->as.string.str.len);
  server_ip_address_cstr[server_ip_address->as.string.str.len] = '\0';

  StringBuilder sb = {0};
  sb_push_i64(&sb, port->as._int);
  sb_push_char(&sb, '\0');

  char *port_cstr = sb.buffer;

  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result;

  if (getaddrinfo(server_ip_address_cstr, port_cstr, &hints, &result) < 0) {
    free(server_ip_address_cstr);
    free(port_cstr);
    return value_unit(vm->current_frame);
  }

  i32 client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (client_socket < 0) {
    free(server_ip_address_cstr);
    free(port_cstr);
    return value_unit(vm->current_frame);
  }

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  i32 connected = connect(client_socket, result->ai_addr, result->ai_addrlen);
  if (connected < 0) {
    free(server_ip_address_cstr);
    free(port_cstr);
    freeaddrinfo(result);
    return value_unit(vm->current_frame);
  }

  free(server_ip_address_cstr);
  free(port_cstr);
  freeaddrinfo(result);

  return value_int(client_socket, vm->current_frame);
}

Value *accept_connection_intrinsic(Vm *vm, Value **args) {
  Value *server = args[0];
  Value *port = args[1];

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port->as._int);

  socklen_t address_size = sizeof(address);
  i32 client_socket = accept(server->as._int,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0)
    return value_unit(vm->current_frame);

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  return value_int(client_socket, vm->current_frame);
}

Value *close_connection_intrinsic(Vm *vm, Value **args) {
  Value *client = args[0];

  close(client->as._int);

  return value_unit(vm->current_frame);
}

Value *send_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *message = args[1];

  send(receiver->as._int, message->as.string.str.ptr,
       message->as.string.str.len, 0);

  return value_unit(vm->current_frame);
}

Value *receive_size_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *size = args[1];

  Str buffer = { arena_alloc(&vm->current_frame->arena, size->as._int), 0 };

  struct pollfd pfd;
  pfd.fd = receiver->as._int;
  pfd.events = POLLIN | POLLERR | POLLHUP;
  poll(&pfd, 1, POLL_TIMEOUT_MS);

  if (pfd.revents != 0)
    buffer.len = recv(receiver->as._int, buffer.ptr, size->as._int, 0);

  if (buffer.len == 0)
    return value_unit(vm->current_frame);

  return value_string(buffer, vm->current_frame);
}

Value *receive_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];

  u32 cap = DEFAULT_RECEIVE_BUFFER_SIZE;
  Str buffer = { arena_alloc(&vm->current_frame->arena, cap), 0 };

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

    if (len < 0)
      return value_unit(vm->current_frame);

    buffer.len += (u32) len;

    if (buffer.len >= cap) {
      char *prev_ptr = buffer.ptr;

      cap += DEFAULT_RECEIVE_BUFFER_SIZE;
      buffer.ptr = arena_alloc(&vm->current_frame->arena, cap);
      memcpy(buffer.ptr, prev_ptr, buffer.len);
    }
  }

  if (buffer.len == 0)
    return value_unit(vm->current_frame);

  return value_string(buffer, vm->current_frame);
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
