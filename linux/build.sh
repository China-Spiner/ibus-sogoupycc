#!/bin/sh
cd $(dirname `readlink -f "$0"`)
rm build -rf
mkdir build/
cd build/
cmake .. -DCMAKE_INSTALL_PREFIX=/usr && make
