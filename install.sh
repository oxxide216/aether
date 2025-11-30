#!/usr/bin/bash

if [ "$DEL" == "" ]; then
  sudo cp aether /usr/bin/
  sudo cp aether-web-setup /usr/bin
  sudo rm -rf /usr/include/aether/
  sudo mkdir -p /usr/include/aether/std/
  sudo mkdir -p /usr/include/aether/load/
  sudo cp std/ae-src/* /usr/include/aether/std/
  sudo cp ae-src/* /usr/include/aether/load

  if [ "$WASM" != "" ]; then
    sudo mkdir /usr/include/aether/wasm/
    sudo cp -r dest/aether.html dest/aether.css dest/aether.js dest/aether.wasm \
               dest/aether.data dest/loader.abc dest/std /usr/include/aether/wasm/
  fi
else
  sudo rm -/usr/bin/aether /usr/bin/aether-web-setup
  sudo rm -rf /usr/include/aether
fi
