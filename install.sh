#!/usr/bin/sh

sudo cp aether /usr/bin/
sudo rm -rf /usr/include/aether/
sudo mkdir -p /usr/include/aether/std/
sudo cp -r $(find std -name "*.ae") /usr/include/aether/std/
