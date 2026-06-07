cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER clang++)
project(LICMPass)

find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_library(LLVMLicmPass MODULE LICMPass.cpp)
target_include_directories(LLVMLicmPass PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(LLVMLicmPass PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(LLVMLicmPass PRIVATE -fno-rtti)
set_target_properties(LLVMLicmPass PROPERTIES CXX_STANDARD 17)
