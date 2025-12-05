#!/usr/bin/bash

# Compiler
CC=cc
OUT=aether
CFLAGS="-Wall -Wextra -Icompiler-lib-include -Ivm-lib-include -Iir-include \
        -Ilibs -Imisc -lm"
BUILD_FLAGS="${@:1}"
LDFLAGS="-z execstack"
SRC="src/main.c"
COMPILER_SRC="$(find compiler-lib-src -name "*.c")"
VM_SRC="$(find vm-lib-src -name "*.c")"
IR_SRC="$(find ir-src -name "*.c") $(find misc -name "*.c")"
LIB_SRC="$(find libs/lexgen/runtime-src -name "*.c")"
STD_SRC="src/std/core.c src/std/math.c src/std/str.c"

if [ "$AETHER_GRAPHICS" == "" ]; then
  AETHER_GRAPHICS=x11
fi

if [ "$NOSYSTEM" == "" ]; then
  STD_SRC="$STD_SRC src/std/base.c src/std/io.c \
                    src/std/net.c src/std/path.c \
                    src/std/term.c src/std/system.c"
else
  CFLAGS="$CFLAGS -DNOSYSTEM"
fi

if [ "$GLASS" != "" ]; then
  CFLAGS="$CFLAGS -DGLASS -Ilibs/winx/include -Ilibs/glass/include"
  LIB_SRC="$LIB_SRC $(find libs/winx/src -name "*.c" -not -name "io.c" \
                           -not -path "libs/winx/src/platform/*")"
  LIB_SRC="$LIB_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC src/std/glass/*"
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

$CC -o $OUT $COMPILER_SRC $VM_SRC $IR_SRC $LIB_SRC \
            $STD_SRC $SRC $CFLAGS $LDFLAGS $BUILD_FLAGS

if [ "$WASM" != "" ]; then
  rm -rf dest/
  mkdir dest/
  cp js-src/aether-web.js dest/

  if [ "$EMCC_PATH" != "" ]; then
    PATH="$PATH:$EMCC_PATH:$EMCC_PATH/upstream/emscripten:$EMCC_PATH/upstream/bin"
  fi

  CC=emcc
  OUT=dest/aether.js
  CFLAGS="$CFLAGS -DEMSCRIPTEN \
                  -s EXPORTED_RUNTIME_METHODS='cwrap' \
                  -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='\$stringToNewUTF8' \
                  -s WASM=1 -s MEMORY64 -s FULL_ES3=1 \
                  -s USE_WEBGL2=1 -s USE_GLFW=0 \
                  -s ENVIRONMENT=web -s SINGLE_FILE=1"
  LDFLAGS=
  SRC="src/emscripten-main.c"
  STD_SRC="$STD_SRC src/std/web.c"

  $CC -o $OUT $COMPILER_SRC $VM_SRC $IR_SRC $LIB_SRC \
              $STD_SRC $SRC $CFLAGS $LDFLAGS $BUILD_FLAGS
fi
