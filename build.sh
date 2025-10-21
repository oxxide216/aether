#!/usr/bin/sh

# Compiler
CFLAGS="-Wall -Wextra -Icompiler-lib-include -Ivm-lib-include -Iir-include \
        -Ilibs -Istd/base/include -lm"
LDFLAGS="-z execstack"
BUILD_FLAGS="${@:1}"
SRC="$(find src -name "*.c")"
COMPILER_SRC="$(find compiler-lib-src -name "*.c")"
VM_SRC="$(find vm-lib-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c")"
LEXGEN_RUNTIME_SRC="$(find libs/lexgen/runtime-src -name "*.c")"
LIB_SRC=""
STD_SRC="$(find std/base/src -name "*.c")"

if [ "$NOSYSTEM" == "" ]; then
  CFLAGS="$CFLAGS -Istd/system/include"
  STD_SRC="$STD_SRC $(find std/system/src -name "*.c")"
else
  CFLAGS="$CFLAGS -DNOSYSTEM"
fi

if [ "$IUI" == "" ]; then
  CFLAGS="$CFLAGS -DNOIUI"
else
  CFLAGS="$CFLAGS -Ilibs/winx/include -Ilibs/glass/include -Istd/iui/include"
  LDFLAGS="$LDFLAGS -lX11 -lGL -lGLEW"
  LIB_SRC="$LIB_SRC $(find libs/winx/src -name "*.c" -not -name "io.c")"
  LIB_SRC="$LIB_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC $(find std/iui/src -name "*.c")"

  xxd -i std/iui/fonts/JetBrainsMono-Regular.ttf > \
    std/iui/include/iui/fonts/JetBrainsMono-Regular.h
fi

lexgen compiler-src/grammar.h compiler-src/grammar.lg
cc -o aether $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $COMPILER_SRC $VM_SRC \
             $IR_SRC $LEXGEN_RUNTIME_SRC $LIB_SRC $STD_SRC
