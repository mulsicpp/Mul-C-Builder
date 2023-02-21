#! /bin/bash

rm -f mul-c

cmake -S src -B build
cmake --build build --config Release

cp build/Release/mul-c .