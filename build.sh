#!/usr/bin/sh

# Compiler
CFLAGS="-Wall -Wextra -Icompiler-lib-include -Ivm-lib-include -Iir-include \
        -Ilibs -Istd/include -Imisc -lm"
LDFLAGS="-z execstack"
BUILD_FLAGS="${@:1}"
SRC="src/main.c"
COMPILER_SRC="$(find compiler-lib-src -name "*.c")"
VM_SRC="$(find vm-lib-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c") $(find misc -name "*.c")"
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

if [ "$GLASS" != "" ]; then
  CFLAGS="$CFLAGS -DGLASS -Ilibs/winx/include -Ilibs/glass/include"
  LDFLAGS="$LDFLAGS -lX11 -lGL -lGLEW"
  LIB_SRC="$LIB_SRC $(find libs/winx/src -name "*.c" -not -name "io.c")"
  LIB_SRC="$LIB_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC std/src/glass/*"
fi

lexgen compiler-lib-src/grammar.h compiler-lib-src/grammar.lg

if [ "$WASM" != "" ]; then
  rm -rf dest/
  mkdir -p dest/std/
  cp std/ae-src/* dest/std/
  cp ems-loader.ae assets/* dest/

  CC=/usr/lib/emsdk/upstream/emscripten/emcc
  OUT=dest/aether.js
  CFLAGS="$CFLAGS -D__emscripten__ --preload-file dest \
              -s EXPORTED_FUNCTIONS=_emscripten_main \
              -s EXPORTED_RUNTIME_METHODS='FS','ccall','cwrap'"
  LDFLAGS=
  SRC="src/emscripten-main.c"
else
  CC=cc
  OUT=aether
fi

if [ "$LIB" == "" ]; then
  $CC -o $OUT $CFLAGS $LDFLAGS $BUILD_FLAGS $SRC $COMPILER_SRC $VM_SRC \
              $IR_SRC $LEXGEN_RUNTIME_SRC $LIB_SRC $STD_SRC
else
  $CC -o libaether.so -shared -fpic -Wl,-z,noexecstack \
      $CFLAGS $LDFLAGS $BUILD_FLAGS src/aether-main.c $COMPILER_SRC $VM_SRC \
      $IR_SRC $LEXGEN_RUNTIME_SRC $LIB_SRC $STD_SRC
fi
