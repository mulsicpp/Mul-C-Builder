del mul-c.exe

cmake -S src -B build
cmake --build build --config Release

copy build\Release\mul-c.exe .