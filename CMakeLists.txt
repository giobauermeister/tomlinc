cmake_minimum_required(VERSION 3.10)
project(TOMLinC)

# Set the output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add include directory
include_directories(include)

# Add the library sources
add_library(tomlinc STATIC
    src/tomlinc.c
    src/tomlinc_internal.c
)

# Add the example directory
add_subdirectory(example)
