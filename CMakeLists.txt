cmake_minimum_required(VERSION 3.0)

project(spack CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")
add_executable(${PROJECT_NAME} main.cpp)