#!/usr/bin/sh

./build-compiler.sh && ./aetherc test.ac $1
./build-vm.sh && ./aether-vm test.ac
