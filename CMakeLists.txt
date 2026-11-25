cmake_minimum_required(VERSION 4.3)
project(memory_allocator)

set(CMAKE_CXX_STANDARD 20)

add_executable(memory_allocator main.cpp
               allocators/allocators.h)

add_executable(allocator_bench benchmarks/benchmark.cpp)
target_include_directories(allocator_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
