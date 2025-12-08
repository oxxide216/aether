#!/usr/bin/bash

# Compiler
CC=cc
OUT=aether
CFLAGS="-Wall -Wextra -Iinclude -Ilibs \
        -Ilibs/winx/include -Ilibs/glass/include -lm"
BUILD_FLAGS="${@:1}"
LDFLAGS="-z execstack"
BIN_SRC="src/main.c"
LIB_SRC="$(find src/lib -name "*.c")"
STD_SRC="src/std/core.c src/std/math.c src/std/str.c"
LIBS_SRC="$(find libs/lexgen/runtime-src -name "*.c")"

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
  LIBS_SRC="$LIBS_SRC $(find libs/winx/src -name "*.c" -not -name "io.c" \
                            -not -path "libs/winx/src/platform/*")"
  LIBS_SRC="$LIBS_SRC $(find libs/glass/src -name "*.c" -not -name "io.c")"
  STD_SRC="$STD_SRC src/std/glass/*"
  if [ "$AETHER_GRAPHICS" == "x11" ]; then
    CFLAGS="$CFLAGS -DX11"
    LDFLAGS="$LDFLAGS -lX11 -lGL -lGLEW"
    LIBS_SRC="$LIBS_SRC $(find libs/winx/src/platform/x11 -name "*.c")"
  elif [ "$AETHER_GRAPHICS" == "web" ]; then
    CFLAGS="$CFLAGS -DWEB"
    LIBS_SRC="$LIBS_SRC $(find libs/winx/src/platform/web -name "*.c")"
  fi
fi

lexgen src/lib/grammar.h src/lib/grammar.lg

$CC -o $OUT $BIN_SRC $MISC_SRC $LIB_SRC $STD_SRC \
            $LIBS_SRC $CFLAGS $LDFLAGS $BUILD_FLAGS

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
  BIN_SRC="src/emscripten-main.c"
  STD_SRC="$STD_SRC src/std/web.c"

  $CC -o $OUT $BIN_SRC $MISC_SRC $LIB_SRC $STD_SRC \
              $LIBS_SRC $CFLAGS $LDFLAGS $BUILD_FLAGS
fi
