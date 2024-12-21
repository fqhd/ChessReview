cmake_minimum_required(VERSION 3.10)

# Project name
project(ChessReview)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the source directory
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Collect all source files in the src directory
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c")

# Add the executable
add_executable(ChessReview ${SRC_FILES})
