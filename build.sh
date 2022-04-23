#!/bin/sh

mkdir -p build

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=\"-static\"
make
