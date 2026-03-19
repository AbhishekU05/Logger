cmake_minimum_required(VERSION 3.16)
project(logger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell compiler where headers are
include_directories(include)

# Create executable
add_executable(logger
    src/main.cpp
    src/logger.cpp
)

# Link pthread (required for threads)
find_package(Threads REQUIRED)
target_link_libraries(logger PRIVATE Threads::Threads)
