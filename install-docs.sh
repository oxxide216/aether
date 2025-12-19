#!/usr/bin/bash

rm -r docs/std docs/aether.js docs/aether-web.js docs/app.abc docs/app.abm
cp -r /usr/include/aether/wasm/* docs/
cp dest/aether.js docs/
cp js-src/aether-web.js docs/
aether --no-dce -c docs/app.abc docs/main.ae
aether --no-dce -m docs/app.abm docs/main.ae
