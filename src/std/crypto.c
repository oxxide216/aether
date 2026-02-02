#include "sodium.h"
#include "sodium/crypto_generichash.h"

#include "aether/vm.h"
#include "aether/misc.h"

Value *hash_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  Bytes result = {
    arena_alloc(&vm->current_frame->arena, crypto_generichash_BYTES),
    crypto_generichash_BYTES,
  };

  crypto_generichash(result.ptr, crypto_generichash_BYTES,
                     (u8 *) message->as.string.str.ptr,
                     message->as.string.str.len, NULL, 0);

  return value_bytes(result, vm->current_frame);
}

Intrinsic crypto_intrinsics[] = {
  { STR_LIT("crypto/hash"), true, 1, { ValueKindString }, &hash_intrinsic, NULL },
};

u32 crypto_intrinsics_len = ARRAY_LEN(crypto_intrinsics);
