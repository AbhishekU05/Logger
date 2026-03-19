cmake_minimum_required(VERSION 3.16)
project(logger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O2 -pthread")

include_directories(include)

add_executable(main src/main.cpp src/logger.cpp src/queue.cpp)
add_executable(bench bench/bench.cpp src/logger.cpp src/queue.cpp)
