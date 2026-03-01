#include "blake2.h"
#include "aether/vm.h"
#include "aether/misc.h"

Value *crypto_hash_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  Bytes result = {
    arena_alloc(&vm->current_frame->arena, BLAKE2B_OUTBYTES),
    BLAKE2B_OUTBYTES,
  };

  blake2b(result.ptr, BLAKE2B_OUTBYTES,
          message->as.string.str.ptr,
          message->as.string.str.len, NULL, 0);

  return value_bytes(result, vm->current_frame);
}

Intrinsic crypto_intrinsics[] = {
  { STR_LIT("crypto/hash"), true, 1, { ValueKindString }, &crypto_hash_intrinsic, NULL },
};

u32 crypto_intrinsics_len = ARRAY_LEN(crypto_intrinsics);
