# Aether

Lisp-like interpreted functional programming language.

Still in active development, only for Linux and WebAssembly for now.

## Features

- **WASM VM**: Runs anywhere WebAssembly runs (browsers, Node.js)
- **Smart Memory**: Automatic arena allocation (no GC pauses, no manual management)
- **Lisp Macros**: Full hygienic macro system
- **Graphics API**: Built-in OpenGL 2D rendering
- **REPL**: Interactive development environment

## Dependencies

- **C compiler**
- **Bash**

That's it!

## Quick start

### Linux

```shell
./build.sh
./install.sh
echo "(println 'Hello, World!')" > main.ae
aether main.ae
```

### Repl
```shell
aether
```

## WebAssembly

### Build Aether

#### Linux

```shell
EMCC_PATH=[emcc] WASM=on ./build.sh
WASM=on ./install.sh
```

Where:
- [emcc] - your Emscripten installation path

### Build your project

```shell
aether-web-setup [out] [in] [source]
```

Where:
- [out] - directory for generated bundle
- [in] - directory for your .html|.css|.js files and assets
- [source] - your main aether source file (main.ae)

Then just start http server inside of [out] directory!

## Build-in graphics

#### Linux

```shell
GLASS=on ./build.sh
./install.sh
```

## Examples

See [examples](examples) directory!

## Screenshots

![Pong game implemented in Aether](screenshots/pong.png)
