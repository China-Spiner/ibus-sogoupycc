#!/bin/sh
rm build -rf
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make
