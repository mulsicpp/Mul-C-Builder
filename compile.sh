#! /bin/bash

g++ -o mulC -std=c++17 -I python3.8_linux/include src/main.cpp src/Builder.cpp src/CLIOptions.cpp -Lpython3.8_linux/lib -lpython3.8