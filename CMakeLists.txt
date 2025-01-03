cmake_minimum_required(VERSION 3.10)

# Project name
project(ChessReview)

find_package(OpenSSL REQUIRED)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the source directory
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Collect all source files in the src directory
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c")

# Add the SFML submodule
set(SFML_DIR ${CMAKE_SOURCE_DIR}/external/SFML)
add_subdirectory(${SFML_DIR})

# Add the executable
add_executable(ChessReview ${SRC_FILES})

# Include SFML headers
target_include_directories(ChessReview PRIVATE ${SFML_DIR}/include)
target_include_directories(ChessReview PRIVATE external/asio/include)

# Link SFML libraries
target_link_libraries(ChessReview PRIVATE sfml-graphics sfml-window sfml-system sfml-network OpenSSL::SSL OpenSSL::Crypto)
