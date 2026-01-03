#!/usr/bin/bash

PREFIX="/usr/local"

if [ "$DEL" == "" ]; then
  sudo cp aether "$PREFIX/bin"
  sudo cp aether-web-setup "$PREFIX/bin"
  sudo rm -rf "$PREFIX/include/aether"
  sudo mkdir -p "$PREFIX/include/aether"
  sudo cp -r ae-src/std/ "$PREFIX/include/aether"

  sudo aether -c "$PREFIX/aether/loader.abc" ae-src/loader.ae

  if [ "$WASM" != "" ]; then
    sudo cp -r dest/ "$PREFIX/include/aether/wasm"
  fi
else
  sudo rm "$PREFIX/bin/aether" "$PREFIX/bin/aether-web-setup"
  sudo rm -rf "$PREFIX/include/aether"
fi
