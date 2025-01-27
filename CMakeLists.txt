# Minumum requirement
cmake_minimum_required(VERSION 3.10)

# Define the name of the project
project(GameBoyEmulator)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 package
find_package(SDL2 REQUIRED)

# Copy the "Roms" folder to the built path
file(COPY ${CMAKE_SOURCE_DIR}/Roms DESTINATION ${CMAKE_BINARY_DIR})

# Include the "src" file
add_subdirectory(src)
