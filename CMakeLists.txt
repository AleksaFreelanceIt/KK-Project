cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER clang++)
project(LLVMPasses)

add_subdirectory(LICM)
add_subdirectory(LoopUnrolling)
