#!/usr/bin/bash

if [ "$DEL" == "" ]; then
  sudo cp aether /usr/bin/
  sudo cp aether-web-setup /usr/bin
  sudo rm -rf /usr/include/aether/
  sudo mkdir -p /usr/include/aether/
  sudo cp -r ae-src/std/ /usr/include/aether/

  sudo aether -c /usr/include/aether/loader.abc ae-src/loader.ae

  if [ "$WASM" != "" ]; then
    sudo cp -r dest/ /usr/include/aether/wasm/
  fi
else
  sudo rm /usr/bin/aether /usr/bin/aether-web-setup
  sudo rm -rf /usr/include/aether/
fi
