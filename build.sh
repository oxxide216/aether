#!/usr/bin/bash

# Compiler
CC=cc
OUT=aether
CFLAGS="-Wall -Wextra -Icompiler-lib-include -Ivm-lib-include -Iir-include \
        -Ilibs -Istd/include -Imisc -lm"

if [ "$WASM" != "" ]; then
  BUILD_FLAGS="${@:2}"
else
  BUILD_FLAGS="${@:1}"
fi

LDFLAGS="-z execstack"
SRC="src/main.c"
COMPILER_SRC="$(find compiler-lib-src -name "*.c")"
VM_SRC="$(find vm-lib-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c") $(find misc -name "*.c")"
LEXGEN_RUNTIME_SRC="$(find libs/lexgen/runtime-src -name "*.c")"
LIB_SRC=""
STD_SRC="std/src/core.c std/src/math.c std/src/str.c"

if [ "$AETHER_GRAPHICS" == "" ]; then
  AETHER_GRAPHICS=x11
fi

if [ "$NOSYSTEM" == "" ]; then
  STD_SRC="$STD_SRC std/src/base.c std/src/io.c \
                    std/src/net.c std/src/path.c \
                    std/src/term.c std/src/system.c"
else
  CFLAGS="$CFLAGS -DNOSYSTEM"
fi

if [ "$GLASS" != "" ]; then
  CFLAGS="$CFLAGS -DGLASS -Ilibs/winx/include -Ilibs/glass/include"
  LIB_SRC="$LIB_SRC $(find libs/winx/src -name "*.c" -not -name "io.c" \
                           -not -path "libs/winx/src/platform/*")"
  LIB_SRC="$LIB_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC std/src/glass/*"
  if [ "$AETHER_GRAPHICS" == "x11" ]; then
    CFLAGS="$CFLAGS -DX11"
    LDFLAGS="$LDFLAGS -lX11 -lGL -lGLEW"
    LIB_SRC="$LIB_SRC $(find libs/winx/src/platform/x11 -name "*.c")"
  elif [ "$AETHER_GRAPHICS" == "web" ]; then
    CFLAGS="$CFLAGS -DWEB"
    LIB_SRC="$LIB_SRC $(find libs/winx/src/platform/web -name "*.c")"
  fi
fi

lexgen compiler-lib-src/grammar.h compiler-lib-src/grammar.lg

$CC -o $OUT $COMPILER_SRC $VM_SRC $IR_SRC $LEXGEN_RUNTIME_SRC \
            $LIB_SRC $STD_SRC $SRC $CFLAGS $LDFLAGS $BUILD_FLAGS

if [ "$WASM" != "" ]; then
  rm -rf dest/
  mkdir -p dest/std/
  cp std/ae-src/* dest/std/
  cp assets/* dest/

  CC=/usr/lib/emsdk/upstream/emscripten/emcc
  OUT=dest/aether.js
  CFLAGS="$CFLAGS -D__emscripten__ --preload-file dest \
                  -s EXPORTED_RUNTIME_METHODS='cwrap' \
                  -s WASM=1 -s MEMORY64 -s FULL_ES3=1 \
                  -s USE_WEBGL2=1 -s USE_GLFW=0 -gsource-map"
  LDFLAGS=
  SRC="src/emscripten-main.c"

  ./aether -o dest/loader.abc ae-src/loader.ae

  if [ "$1" != "" ]; then
    ./aether -o dest/app.abc $1
  fi

  $CC -o $OUT $COMPILER_SRC $VM_SRC $IR_SRC $LEXGEN_RUNTIME_SRC \
              $LIB_SRC $STD_SRC $SRC $CFLAGS $LDFLAGS $BUILD_FLAGS
fi
