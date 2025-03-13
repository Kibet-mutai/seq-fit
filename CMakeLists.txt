cmake_minimum_required(VERSION 3.30.0)

set(CMAKE_CXX_STANDARD 17)
project(seqfit)

#VERSION 2 of mem-mgr
project(seqfit_v2)

add_executable(seqfit src/mem-mgr.cpp )
add_executable(seqfit_v2 src/seq_fit_v2.cpp)
