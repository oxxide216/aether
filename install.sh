#!/usr/bin/bash

sudo cp aether /usr/bin/
sudo rm -rf /usr/include/aether/
sudo mkdir -p /usr/include/aether/std/
sudo cp std/ae-src/* /usr/include/aether/std/
sudo cp loader.ae ems-loader.ae repl.ae /usr/include/aether/
