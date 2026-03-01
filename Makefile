.RECIPEPREFIX = >

CC = cc
EMCC = emcc
CFLAGS = -Wall -Wextra -Iinclude -Ilibs -Ilibs/lexgen/include
LDFLAGS = -lm
EMCFLAGS = $(CFLAGS)
EMLDFLAGS = $(LDFLAGS) \
            -s EXPORTED_RUNTIME_METHODS=['cwrap','HEAPU8'] \
            -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
            -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$$stringToNewUTF8' \
            -s WASM=1 -s FULL_ES3=1 \
            -s USE_WEBGL2=1 -s USE_GLFW=0 \
            -s ENVIRONMENT=web -s SINGLE_FILE=1 \
            -s FETCH=1
EMFLAGS = -s MEMORY64
BUILD_DIR = build

SRC = $(wildcard src/lib/*.c) \
      src/std/core.c src/std/math.c \
      src/std/str.c src/std/random.c
MAIN_SRC = $(SRC) src/main.c
WASM_SRC = $(SRC) src/emscripten-main.c src/std/web.c
LIBS_SRC = $(wildcard libs/lexgen/src/runtime/*.c) libs/lexgen/src/common/wstr.c

OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(MAIN_SRC))
LIBS_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIBS_SRC))

WASM_OBJ = $(patsubst src/%.c,$(BUILD_DIR)/wasm/%.o,$(WASM_SRC))
WASM_LIBS_OBJ = $(patsubst %.c,$(BUILD_DIR)/wasm/%.o,$(LIBS_SRC))

ifdef EMCC_PATH
  PATH := $(EMCC_PATH):$(EMCC_PATH)/upstream/emscripten:$(EMCC_PATH)/upstream/bin:$(PATH)
endif

ifndef PREFIX
  PREFIX = /usr/local
endif

ifdef NDEBUG
  CFLAGS += -DNDEBUG
endif

ifdef NOSYSTEM
  CFLAGS += -DNOSYSTEM
else
  SRC += src/std/base.c src/std/io.c \
         src/std/net.c src/std/path.c \
         src/std/term.c src/std/system.c
endif

ifdef GLASS
  CFLAGS += -DGLASS -Ilibs/winx/include -Ilibs/glass/include
  LDFLAGS += -lX11 -lGL -lGLEW
  SRC += $(wildcard src/std/glass/*.c)
  LIBS_SRC += $(wildcard libs/glass/src/*.c) \
              $(wildcard libs/winx/src/*.c) \
              $(wildcard libs/winx/src/platform/x11/*.c)
endif

ifdef CRYPTO
  CFLAGS += -DCRYPTO -Ilibs/blake2/ref
  SRC += src/std/crypto.c
  LIBS_SRC += libs/blake2/ref/blake2b-ref.c
endif

ifdef TLS
  CFLAGS += -DLIBTLS -DTLS_AMALGAMATION -Ilibs/tlse
  SRC += src/std/tls/tls.c
endif

aether: $(OBJ) $(LIBS_OBJ)
> $(CC) $(LDFLAGS) -o aether $(OBJ) $(LIBS_OBJ)

$(BUILD_DIR)/%.o: src/%.c
> mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/libs/%.o: libs/%.c
> mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -c -o $@ $^

wasm: $(WASM_OBJ) $(WASM_LIBS_OBJ)
> $(EMCC) $(EMFLAGS) $(EMLDFLAGS) -o dest/aether.js $(WASM_OBJ) $(WASM_LIBS_OBJ)

$(BUILD_DIR)/wasm/%.o: src/%.c
> mkdir -p $(dir $@)
> $(EMCC) $(EMFLAGS) $(EMCFLAGS) -c -o $@ $^

$(BUILD_DIR)/wasm/libs/%.o: libs/%.c
> mkdir -p $(dir $@)
> $(EMCC) $(EMFLAGS) $(EMCFLAGS) -c -o $@ $^

install: aether $(PREFIX)/include/aether
> cp aether $(PREFIX)/bin

$(PREFIX)/include/aether: aether
> rm -rf $(PREFIX)/include/aether
> mkdir -p $(PREFIX)/include/aether
> cp -r ae-src/std $(PREFIX)/include/aether
> ./aether -c $(PREFIX)/include/aether/loader.abc ae-src/loader.ae

wasm-install: $(PREFIX)/include/aether dest/aether.js
> cp aether-web-setup $(PREFIX)/bin
> rm -rf $(PREFIX)/include/aether/wasm
> cp -r dest $(PREFIX)/include/aether/wasm
> cp js-src/aether-web.js $(PREFIX)/include/aether/wasm

uninstall:
> rm -rf $(PREFIX)/bin/aether $(PREFIX)/bin/aether-web-setup $(PREFIX)/include/aether

clean:
> rm -rf $(BUILD_DIR) aether dest
