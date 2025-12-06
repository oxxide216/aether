#!/usr/bin/bash

aether-web-setup example-dest/
aether -c example-dest/app.abc main.ae
cp res/* example-dest/
cd example-dest/
python -m http.server 8080
