cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(mul-c-build)


add_executable(mul-c-build main.cpp)

target_link_libraries(mul-c-build PUBLIC ../Python311/libs/python311 ../Python311/libs/python3)