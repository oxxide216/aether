#!/usr/bin/sh

# Compiler
CFLAGS="-Wall -Wextra -Icompiler-lib-include -Ivm-lib-include -Iir-include \
        -Ilibs -Istd/include -lm"
LDFLAGS="-z execstack"
BUILD_FLAGS="${@:1}"
SRC="$(find src -name "*.c")"
COMPILER_SRC="$(find compiler-lib-src -name "*.c")"
VM_SRC="$(find vm-lib-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c")"
LEXGEN_RUNTIME_SRC="$(find libs/lexgen/runtime-src -name "*.c")"
LIB_SRC=""
STD_SRC="std/src/core.c std/src/math.c std/src/str.c"

if [ "$NOSYSTEM" == "" ]; then
  STD_SRC="$STD_SRC std/src/base.c std/src/io.c \
                    std/src/net.c std/src/path.c \
                    std/src/term.c std/src/system.c"
else
  CFLAGS="$CFLAGS -DNOSYSTEM"
fi

if [ "$IUI" != "" ]; then
  CFLAGS="$CFLAGS -DIUI -Ilibs/winx/include -Ilibs/glass/include -Istd/iui/include"
  LDFLAGS="$LDFLAGS -lX11 -lGL -lGLEW"
  LIB_SRC="$LIB_SRC $(find libs/winx/src -name "*.c" -not -name "io.c")"
  LIB_SRC="$LIB_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC $(find std/src/iui -name "*.c")"

  xxd -i std/src/iui/fonts/JetBrainsMono-Regular.ttf > \
    std/src/iui/fonts/JetBrainsMono-Regular.h
fi

lexgen compiler-lib-src/grammar.h compiler-lib-src/grammar.lg
cc -o aether $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $COMPILER_SRC $VM_SRC \
             $IR_SRC $LEXGEN_RUNTIME_SRC $LIB_SRC $STD_SRC
