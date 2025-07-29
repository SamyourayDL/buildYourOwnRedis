cmake_minimum_required(VERSION 3.31)
project(Redis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(redis main.cpp)

