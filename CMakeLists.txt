cmake_minimum_required(VERSION 3.30.0)

set(CMAKE_CXX_STANDARD 17)
project(seqfit)

add_executable(${CMAKE_PROJECT_NAME} src/mem-mgr.cpp)
