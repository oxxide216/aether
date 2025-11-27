#!/usr/bin/bash

sudo cp aether /usr/bin/
sudo rm -rf /usr/include/aether/
sudo mkdir -p /usr/include/aether/std/
sudo mkdir -p /usr/include/aether/load/
sudo cp std/ae-src/* /usr/include/aether/std/
sudo cp ae-src/* /usr/include/aether/load
