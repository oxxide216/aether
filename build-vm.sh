#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Ilib-vm-include -Iir-include -Istd/base/include"
LDFLAGS="-z execstack"
BUILD_FLAGS="${@:1}"
SRC="$(find vm-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c" -not -name "serializer.*")"
LIB_SRC="$(find lib-vm-src -name "*.c" -not -name "io.c")"
STD_SRC="$(find std/base/src -name "*.c")"

if [ "$NOSYSTEM" == "" ]; then
  CFLAGS="$CFLAGS -Istd/system/include"
  STD_SRC="$STD_SRC $(find std/system/src -name "*.c")"
else
  CFLAGS="$CFLAGS -DNOSYSTEM"
fi

if [ "$NOIUI" == "" ]; then
  CFLAGS="$CFLAGS -DIUI -Ilibs/winx/include -Ilibs/glass/include \
          -Istd/iui/include"
  LDFLAGS="$LDFLAGS -lX11 -lGL -lGLEW -lm"
  LIB_SRC="$LIB_SRC $(find libs/winx/src -name "*.c" -not -name "io.c")"
  LIB_SRC="$LIB_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC $(find std/iui/src -name "*.c")"

  xxd -i std/iui/fonts/JetBrainsMono-Regular.ttf > \
    std/iui/include/iui/fonts/JetBrainsMono-Regular.h
fi

cc -o aether-vm $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $LIB_SRC $IR_SRC $STD_SRC
