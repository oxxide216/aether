#include <netinet/tcp.h>
#include <poll.h>

#include "tlse-server.h"
#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_RECEIVE_BUFFER_SIZE 64

extern Value *create_server_intrinsic(Vm *vm, Value **args);

Da(struct TLSContext *) ctxs = {0};

Value *create_tls_server_intrinsic(Vm *vm, Value **args) {
  Value *socket = create_server_intrinsic(vm, args);
  Value *cert = args[1];
  Value *key = args[2];

  Dict *server_dict = arena_alloc(&vm->current_frame->arena, sizeof(Dict));
  memset(server_dict, 0, sizeof(Dict));

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  dict_set_value(vm->current_frame, server_dict, socket_key, socket);

  Value *ctx_id_key = value_string(STR_LIT("ctx-id"), vm->current_frame);
  dict_set_value(vm->current_frame, server_dict,
                 ctx_id_key, value_int(ctxs.len, vm->current_frame));

  struct TLSContext *ctx = tls_create_context(1, TLS_V12);
  DA_APPEND(ctxs, ctx);

  tls_load_certificates(ctx, (u8 *) cert->as.string.str.ptr, cert->as.string.str.len);
  tls_load_private_key(ctx, (u8 *) key->as.string.str.ptr, key->as.string.str.len);

  return value_dict(server_dict, vm->current_frame);
}

static void send_pending(i32 socket, struct TLSContext *ctx) {
  u32 buffer_cap = 0;
  u32 buffer_index = 0;
  const u8 *buffer = tls_get_write_buffer(ctx, &buffer_cap);

  while (buffer && buffer_cap > 0) {
    i32 result = send(socket, (char *) &buffer[buffer_index], buffer_cap, 0);
    if (result <= 0)
      break;

    buffer_cap -= result;
    buffer_index += result;
  }

  tls_buffer_clear(ctx);
}

static i32 verify_signature(struct TLSContext *ctx, struct TLSCertificate **cert_chain, i32 len) {
  (void) ctx;
  (void) cert_chain;
  (void) len;

  return no_error;
}

Value *close_tls_connection_intrinsic(Vm *vm, Value **args) {
  Value *client = args[0];

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  Value *socket = dict_get_value(client->as.dict, socket_key);

  if (socket->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  Value *ctx_id_key = value_string(STR_LIT("ctx-id"), vm->current_frame);
  Value *ctx_id = dict_get_value(client->as.dict, ctx_id_key);

  if (ctx_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  tls_close_notify(ctxs.items[ctx_id->as._int]);
  send_pending(socket->as._int, ctxs.items[ctx_id->as._int]);

  close(socket->as._int);

  tls_destroy_context(ctxs.items[ctx_id->as._int]);

  return value_unit(vm->current_frame);
}

Value *accept_tls_connection_intrinsic(Vm *vm, Value **args) {
  Value *server = args[0];
  Value *port = args[1];

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port->as._int);

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  Value *server_socket = dict_get_value(server->as.dict, socket_key);

  if (server_socket->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  Value *server_ctx_id_key = value_string(STR_LIT("ctx-id"), vm->current_frame);
  Value *server_ctx_id = dict_get_value(server->as.dict, server_ctx_id_key);

  if (server_ctx_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  socklen_t address_size = sizeof(address);
  i32 client_socket = accept(server_socket->as._int,
                             (struct sockaddr*) &address,
                             &address_size);
  if (client_socket < 0)
    return value_unit(vm->current_frame);

  i32 enable = 1;
  setsockopt(client_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable));

  Dict *client_dict = arena_alloc(&vm->current_frame->arena, sizeof(Dict));
  memset(client_dict, 0, sizeof(Dict));

  dict_set_value(vm->current_frame, client_dict, socket_key,
                 value_int(client_socket, vm->current_frame));

  Value *ctx_id_key = value_string(STR_LIT("ctx-id"), vm->current_frame);
  dict_set_value(vm->current_frame, client_dict, ctx_id_key,
                 value_int(ctxs.len, vm->current_frame));

  struct TLSContext *ctx = tls_accept(ctxs.items[server_ctx_id->as._int]);
  DA_APPEND(ctxs, ctx);

  u8 buffer[64];
  i32 buffer_len = 0;
  while ((buffer_len = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
    if (tls_consume_stream(ctx, buffer, buffer_len, verify_signature) < 0) {
      close(client_socket);
      return value_unit(vm->current_frame);
    }

    send_pending(client_socket, ctx);

    if (tls_established(ctx) == 1)
      break;
  }

  return value_dict(client_dict, vm->current_frame);
}

Value *send_tls_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *message = args[1];

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  Value *socket = dict_get_value(receiver->as.dict, socket_key);

  Value *ctx_id_key = value_string(STR_LIT("ctx-id"), vm->current_frame);
  Value *ctx_id = dict_get_value(receiver->as.dict, ctx_id_key);

  if (ctx_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  if (message->kind == ValueKindString)
    tls_write(ctxs.items[ctx_id->as._int],
              (u8 *) message->as.string.str.ptr,
              message->as.string.str.len);
  else if (message->kind == ValueKindBytes)
    tls_write(ctxs.items[ctx_id->as._int],
              (u8 *) message->as.bytes.ptr,
              message->as.bytes.len);

  send_pending(socket->as._int, ctxs.items[ctx_id->as._int]);

  return value_unit(vm->current_frame);
}

Value *receive_tls_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *timeout = args[1];

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  Value *socket = dict_get_value(receiver->as.dict, socket_key);

  Value *ctx_id_key = value_string(STR_LIT("ctx-id"), vm->current_frame);
  Value *ctx_id = dict_get_value(receiver->as.dict, ctx_id_key);

  if (ctx_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  u8 buffer[256];

  struct pollfd pfd = {0};
  pfd.fd = socket->as._int;
  pfd.events = POLLIN;

  while (true) {
    i32 result = poll(&pfd, 1, timeout->as._int);
    if (result < 0)
      return value_unit(vm->current_frame);

    if (pfd.revents == 0)
      break;

    i32 buffer_len = recv(socket->as._int, buffer, sizeof(buffer), 0);

    if (buffer_len < 0)
      return value_unit(vm->current_frame);

    if (buffer_len == 0)
      break;

    result = tls_consume_stream(ctxs.items[ctx_id->as._int],
                                buffer,
                                buffer_len,
                                verify_signature);

    if (result < 0)
      return value_unit(vm->current_frame);
  }

  u32 final_cap = DEFAULT_RECEIVE_BUFFER_SIZE;
  Bytes final_buffer = { arena_alloc(&vm->current_frame->arena, final_cap), 0 };

  while (true) {
    i32 len = tls_read(ctxs.items[ctx_id->as._int],
                       final_buffer.ptr + final_buffer.len,
                       final_cap - final_buffer.len);

    if (len < 0)
      return value_unit(vm->current_frame);

    if (len == 0)
      break;

    final_buffer.len += (u32) len;

    if (final_buffer.len == final_cap) {
      u8 *prev_ptr = final_buffer.ptr;

      final_cap += DEFAULT_RECEIVE_BUFFER_SIZE;
      final_buffer.ptr = arena_alloc(&vm->current_frame->arena, final_cap);
      memcpy(final_buffer.ptr, prev_ptr, final_buffer.len);
    }
  }

  if (final_buffer.len == 0)
    return value_unit(vm->current_frame);

  return value_bytes(final_buffer, vm->current_frame);
}

Intrinsic tls_intrinsics[] = {
  { STR_LIT("create-tls-server"), true, 3,
    { ValueKindInt, ValueKindString, ValueKindString },
    &create_tls_server_intrinsic, NULL },
  { STR_LIT("accept-tls-connection"), true, 2,
    { ValueKindDict, ValueKindInt },
    &accept_tls_connection_intrinsic, NULL },
  { STR_LIT("close-tls-connection"), false, 1,
    { ValueKindDict },
    &close_tls_connection_intrinsic, NULL },
  { STR_LIT("send-tls"), false, 2,
    { ValueKindDict, ValueKindString },
    &send_tls_intrinsic, NULL },
  { STR_LIT("send-tls"), false, 2,
    { ValueKindDict, ValueKindBytes },
    &send_tls_intrinsic, NULL },
  { STR_LIT("receive-tls"), true, 2,
    { ValueKindDict, ValueKindInt },
    &receive_tls_intrinsic, NULL },
};

u32 tls_intrinsics_len = ARRAY_LEN(tls_intrinsics);
