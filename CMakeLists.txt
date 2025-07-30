cmake_minimum_required(VERSION 3.31)
project(Redis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_executable(server src/server.cpp)
target_compile_options(server PUBLIC -Wall -Wextra -O2)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/lib)

add_executable(client src/client.cpp
        lib/utility.h)
target_compile_options(client PUBLIC -Wall -Wextra -O2)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/lib)

