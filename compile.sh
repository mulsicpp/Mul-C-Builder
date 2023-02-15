#! /bin/bash

rm -f cppbuild.exe

cmake -S src -B build
cmake --build build --config Release

cp build/Release/mul-c .