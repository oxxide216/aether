#!/usr/bin

./build-commpiler && ./aetherc test.abc $1
./build-vm && ./aether-vm test.abc
