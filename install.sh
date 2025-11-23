#!/usr/bin/sh

sudo cp aether /usr/bin/
sudo rm -rf /usr/include/aether/
sudo mkdir -p /usr/include/aether/std/
sudo cp std/ae-src/* /usr/include/aether/std/
sudo cp loader.ae /usr/include/aether/
sudo cp ems-loader.ae /usr/include/aether/
