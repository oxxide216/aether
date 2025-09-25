#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Ivm-include -Iir-include"
LDFLAGS="-z execstack -lX11 -lGL -lGLEW"
BUILD_FLAGS="${@:1}"
SRC="$(find test-vm-src -name "*.c")"
LIB_SRC="$(find vm-src -name "*.c" -not -name "io.c")"
IR_SRC="$(find ir-src -name "*.c" -not -name "serializer.*")"

cc -o aether-vm $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $LIB_SRC $IR_SRC
