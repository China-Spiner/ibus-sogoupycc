#!/bin/sh
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make
