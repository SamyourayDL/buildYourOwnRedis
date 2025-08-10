cmake_minimum_required(VERSION 3.31)
project(Redis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")

add_executable(server src/server_main.cpp src/server.cpp)
target_compile_options(server PUBLIC -Wall -Wextra -O2)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/include)

add_executable(client src/client_main.cpp src/client.cpp)
target_compile_options(client PUBLIC -Wall -Wextra -O2)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/include)

add_library(server_lib src/server.cpp)
target_compile_options(server_lib PUBLIC -Wall -Wextra -O2)
target_include_directories(server_lib PUBLIC ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/include)

add_library(client_lib src/client.cpp)
target_compile_options(client_lib PUBLIC -Wall -Wextra -O2)
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/include)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)