# LLVM LICM & Loop Unrolling Pass

Out-of-tree LLVM pass implementing loop unrolling, designed to run after LICM.

## Requirements

- LLVM 22
- Clang 22
- CMake 3.20+
- Ninja

## Build

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build -G Ninja
cd build && ninja && cd ..
```

## Usage

Compile input to LLVM IR:
```bash
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -S example.c -o example.ll
```

Run passes:
```bash
opt -load-pass-plugin /build/libLLVMLicmPass.so \
    -load-pass-plugin /build/libLLVMLoopUnrollingPass.so \
    -passes="loop(licm-pass,loop-unrolling-pass)" \
    example.ll -o example_out.ll -S
```
