#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Iaether-src -Iair-src"
LDFLAGS="-z execstack -lX11 -lGL -lGLEW"
BUILD_FLAGS="${@:1}"
SRC="$(find aether-src -name "*.c")"
AIR_SRC="$(find air-src -name "*.c" -not -name "serializer.*")"

cc -o aether $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $AIR_SRC
