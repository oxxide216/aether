#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Icompiler-src -Iir-include -Ilibs"
LDFLAGS="-z execstack"
BUILD_FLAGS="${@:1}"
SRC="$(find compiler-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c" -not -name "deserializer.*")"
LEXGEN_RUNTIME_SRC="$(find libs/lexgen/runtime-src -name "*.c")"

lexgen compiler-src/grammar.h compiler-src/grammar.lg
cc -o aetherc $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $IR_SRC $LEXGEN_RUNTIME_SRC
