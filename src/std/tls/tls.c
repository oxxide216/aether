#include <poll.h>


#include "tlse-server.h"
#include "aether/vm.h"
#include "aether/misc.h"

#define DEFAULT_RECEIVE_BUFFER_SIZE 64

extern Value *create_server_intrinsic(Vm *vm, Value **args);
extern Value *accept_connection_intrinsic(Vm *vm, Value **args);

static Da(SSL *) ssls = {0};

static char *str_to_cstr(Str str) {
  char *cstr = malloc(str.len + 1);
  memcpy(cstr, str.ptr, str.len);
  cstr[str.len] = '\0';

  return cstr;
}

Value *create_ssl_server_intrinsic(Vm *vm, Value **args) {
  Value *socket = create_server_intrinsic(vm, args);
  Value *cert_path = args[1];
  Value *key_path = args[2];

  Dict *server_dict = arena_alloc(&vm->current_frame->arena, sizeof(Dict));

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  dict_set_value(vm->current_frame, server_dict, socket_key, socket);

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  dict_set_value(vm->current_frame, server_dict,
                 ssl_id_key, value_int(ssls.len, vm->current_frame));

  SSL *ssl = SSL_CTX_new(SSLv3_server_method());
  DA_APPEND(ssls, ssl);

  char *cert_path_cstr = str_to_cstr(cert_path->as.string.str);
  char *key_path_cstr = str_to_cstr(key_path->as.string.str);

  SSL_CTX_use_certificate_file(ssl, cert_path_cstr, SSL_SERVER_RSA_CERT);
  SSL_CTX_use_PrivateKey_file(ssl, key_path_cstr, SSL_SERVER_RSA_KEY);

  free(cert_path_cstr);
  free(key_path_cstr);

  if (!SSL_CTX_check_private_key(ssl)) {
    close(socket->as._int);
    return value_unit(vm->current_frame);
  }

  return value_dict(server_dict, vm->current_frame);
}

Value *accept_ssl_connection_intrinsic(Vm *vm, Value **args) {
  Value *server = args[0];
  Value *client_socket = accept_connection_intrinsic(vm, args);

  Dict *client_dict = arena_alloc(&vm->current_frame->arena, sizeof(Dict));

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  dict_set_value(vm->current_frame, client_dict, socket_key, client_socket);

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  dict_set_value(vm->current_frame, client_dict,
                 ssl_id_key, value_int(ssls.len, vm->current_frame));

  Value *server_ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  Value *server_ssl_id = dict_get_value(server->as.dict, server_ssl_id_key);

  if (server_ssl_id->kind != ValueKindInt) {
    close(client_socket->as._int);
    return value_unit(vm->current_frame);
  }

  SSL *ssl = SSL_new(ssls.items[server_ssl_id->as._int]);
  DA_APPEND(ssls, ssl);

  SSL_set_fd(ssl, client_socket->as._int);

  if (!SSL_accept(ssl)) {
    close(client_socket->as._int);
    return value_unit(vm->current_frame);
  }

  return value_dict(client_dict, vm->current_frame);
}

Value *close_ssl_client_connection_intrinsic(Vm *vm, Value **args) {
  Value *client = args[0];

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  Value *socket = dict_get_value(client->as.dict, socket_key);

  if (socket->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  Value *ssl_id = dict_get_value(client->as.dict, ssl_id_key);

  if (ssl_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  close(socket->as._int);
  SSL_free(ssls.items[ssl_id->as._int]);

  return value_unit(vm->current_frame);
}

Value *close_ssl_server_connection_intrinsic(Vm *vm, Value **args) {
  Value *server = args[0];

  Value *socket_key = value_string(STR_LIT("socket"), vm->current_frame);
  Value *socket = dict_get_value(server->as.dict, socket_key);

  if (socket->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  Value *ssl_id = dict_get_value(server->as.dict, ssl_id_key);

  if (ssl_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  close(socket->as._int);
  SSL_CTX_free(ssls.items[ssl_id->as._int]);

  return value_unit(vm->current_frame);
}

Value *send_ssl_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *message = args[1];

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  Value *ssl_id = dict_get_value(receiver->as.dict, ssl_id_key);

  if (ssl_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  if (message->kind == ValueKindString)
    SSL_write(ssls.items[ssl_id->as._int],
              message->as.string.str.ptr,
              message->as.string.str.len);
  else if (message->kind == ValueKindBytes)
    SSL_write(ssls.items[ssl_id->as._int],
              message->as.bytes.ptr,
              message->as.bytes.len);

  return value_unit(vm->current_frame);
}

Value *receive_size_ssl_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *size = args[1];
  Value *timeout = args[2];

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  Value *ssl_id = dict_get_value(receiver->as.dict, ssl_id_key);

  if (ssl_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  struct pollfd pfd;
  pfd.fd = receiver->as._int;
  pfd.events = POLLIN;

  i32 result = poll(&pfd, 1, timeout->as._int);
  if (result < 0 || pfd.revents == 0)
    return value_unit(vm->current_frame);

  Bytes buffer;
  buffer.ptr = arena_alloc(&vm->current_frame->arena, size->as._int);
  buffer.len = recv(receiver->as._int, buffer.ptr, size->as._int, 0);

  return value_bytes(buffer, vm->current_frame);
}

Value *receive_ssl_intrinsic(Vm *vm, Value **args) {
  Value *receiver = args[0];
  Value *timeout = args[1];

  Value *ssl_id_key = value_string(STR_LIT("ssl-id"), vm->current_frame);
  Value *ssl_id = dict_get_value(receiver->as.dict, ssl_id_key);

  if (ssl_id->kind != ValueKindInt)
    return value_unit(vm->current_frame);

  u32 cap = DEFAULT_RECEIVE_BUFFER_SIZE;
  Bytes buffer = { arena_alloc(&vm->current_frame->arena, cap), 0 };

  struct pollfd pfd = {0};
  pfd.fd = receiver->as._int;
  pfd.events = POLLIN;

  while (true) {
    i32 result = poll(&pfd, 1, timeout->as._int);
    if (result < 0)
      return value_unit(vm->current_frame);

    if (pfd.revents == 0)
      break;

    i32 len = SSL_read(ssls.items[ssl_id->as._int],
                   buffer.ptr + buffer.len,
                   cap - buffer.len);

    if (len < 0)
      return value_unit(vm->current_frame);

    if (len == 0)
      break;

    buffer.len += (u32) len;

    if (buffer.len == cap) {
      u8 *prev_ptr = buffer.ptr;

      cap += DEFAULT_RECEIVE_BUFFER_SIZE;
      buffer.ptr = arena_alloc(&vm->current_frame->arena, cap);
      memcpy(buffer.ptr, prev_ptr, buffer.len);
    }
  }

  if (buffer.len == 0)
    return value_unit(vm->current_frame);

  return value_bytes(buffer, vm->current_frame);
}

Intrinsic tls_intrinsics[] = {
  { STR_LIT("create-ssl-server"), true, 1,
    { ValueKindInt },
    &create_ssl_server_intrinsic, NULL },
  { STR_LIT("accept-ssl-connection"), true, 2,
    { ValueKindDict, ValueKindInt },
    &accept_ssl_connection_intrinsic, NULL },
  { STR_LIT("close-ssl-client-connection"), false, 1,
    { ValueKindDict },
    &close_ssl_client_connection_intrinsic, NULL },
  { STR_LIT("close-ssl-server-connection"), false, 1,
    { ValueKindDict },
    &close_ssl_server_connection_intrinsic, NULL },
  { STR_LIT("send-ssl"), false, 2,
    { ValueKindDict, ValueKindString },
    &send_ssl_intrinsic, NULL },
  { STR_LIT("send-ssl"), false, 2,
    { ValueKindDict, ValueKindBytes },
    &send_ssl_intrinsic, NULL },
  { STR_LIT("receive-size-ssl"), true, 3,
    { ValueKindDict, ValueKindInt, ValueKindInt },
    &receive_size_ssl_intrinsic, NULL },
  { STR_LIT("receive-ssl"), true, 2,
    { ValueKindDict, ValueKindInt },
    &receive_ssl_intrinsic, NULL },
};

u32 tls_intrinsics_len = ARRAY_LEN(tls_intrinsics);
