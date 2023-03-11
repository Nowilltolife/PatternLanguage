cmake_minimum_required(VERSION 3.16)
project(cgranges)

set(CMAKE_CXX_STANDARD20)

add_library(cgranges INTERFACE)

target_include_directories(cgranges INTERFACE include)
target_compile_options(intervaltree INTERFACE "-DUSE_INTERVAL_TREE_NAMESPACE")