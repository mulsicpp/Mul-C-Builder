cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(mul-c)


add_executable(mul-c main.cpp Builder.cpp CLIOptions.cpp)
target_include_directories(mul-c PUBLIC ../Python311/include)

target_link_libraries(mul-c PUBLIC ../Python311_windows/libs/python311)
