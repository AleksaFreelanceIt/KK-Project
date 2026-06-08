cmake_minimum_required(VERSION 3.20)
project(LoopUnrollingPass)

execute_process(
    COMMAND llvm-config --cmakedir
    OUTPUT_VARIABLE LLVM_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_library(LLVMLoopUnrollingPass MODULE LoopUnrollingPass.cpp)
target_include_directories(LLVMLoopUnrollingPass PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(LLVMLoopUnrollingPass PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(LLVMLoopUnrollingPass PRIVATE -fno-rtti)
set_target_properties(LLVMLoopUnrollingPass PROPERTIES CXX_STANDARD 17)
