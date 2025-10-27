#!/usr/bin/sh

./build.sh && ./install.sh && ./aether ${@:1}
