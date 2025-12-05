#!/usr/bin/bash

if [ "$DEL" == "" ]; then
  sudo cp aether /usr/bin/
  sudo cp aether-web-setup /usr/bin
  sudo rm -rf /usr/include/aether/
  sudo mkdir -p /usr/include/aether/load/
  sudo cp -r ae-src/std/ /usr/include/aether/
  sudo cp ae-src/*.ae /usr/include/aether/load

  if [ "$WASM" != "" ]; then
    sudo cp -r dest/ /usr/include/aether/wasm/
  fi
else
  sudo rm /usr/bin/aether /usr/bin/aether-web-setup
  sudo rm -rf /usr/include/aether/
fi
