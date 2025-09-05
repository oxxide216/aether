#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Iac-src -Iair-src -Ilibs"
LDFLAGS="-z execstack"
BUILD_FLAGS="${@:1}"
SRC="$(find ac-src -name "*.c")"
AIR_SRC="$(find air-src -name "*.c" -not -name "deserializer.*")"
LEXGEN_RUNTIME_SRC="$(find libs/lexgen/runtime-src -name "*.c")"

lexgen ac-src/grammar.h ac-src/grammar.lg
cc -o ac $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $AIR_SRC $LEXGEN_RUNTIME_SRC
