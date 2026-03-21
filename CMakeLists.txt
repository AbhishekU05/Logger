cmake_minimum_required(VERSION 3.16)
project(logger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

option(USE_LOCKFREE "Use lock-free logger" OFF)

if(USE_LOCKFREE)
    add_executable(logger
        src/main.cpp
        src/logger_lockfree.cpp
        src/queue.cpp
    )
    target_compile_definitions(logger PRIVATE USE_LOCKFREE)
else()
    add_executable(logger
        src/main.cpp
        src/logger_mutex.cpp
    )
endif()

find_package(Threads REQUIRED)
target_link_libraries(logger PRIVATE Threads::Threads)
