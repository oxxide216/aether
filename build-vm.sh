#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Ivn-src -Iir-src"
LDFLAGS="-z execstack -lX11 -lGL -lGLEW"
BUILD_FLAGS="${@:1}"
SRC="$(find vm-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c" -not -name "serializer.*")"

cc -o aether-vm $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $IR_SRC
