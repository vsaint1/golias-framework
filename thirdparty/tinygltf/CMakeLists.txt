cmake_minimum_required(VERSION 3.18)

project(tinygltf)

set(CMAKE_C_STANDARD 99)

add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
