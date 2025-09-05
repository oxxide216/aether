#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Iaether-src -Iair-src -Ilibs -Ilibs/winx/src"
LDFLAGS="-z execstack -lX11 -lGL -lGLEW"
BUILD_FLAGS="${@:1}"
SRC="$(find aether-src -name "*.c")"
AIR_SRC="$(find air-src -name "*.c" -not -name "serializer.*")"
WINX_SRC="$(find libs/winx/src -name "*.c")"
GLASS_SRC="$(find libs/glass/src -name "*.c")"

cc -o aether $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $AIR_SRC $WINX_SRC $GLASS_SRC
