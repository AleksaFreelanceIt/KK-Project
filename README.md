# LLVM LICM & Loop Unrolling Passes

LLVM passes implementing Loop Invariant Code Motion (LICM) and Loop Unrolling, built against LLVM 17 using the Legacy Pass Manager.

## Requirements

- LLVM 17 (built from source)
- Clang 17

## Usage

Compile input to LLVM IR:
```bash
./bin/clang -O0 -Xclang -disable-O0-optnone -emit-llvm -S example.c -o example.ll
```

Run LICM pass:
```bash
./bin/opt -enable-new-pm=0 -load lib/LLVMLicmPass.so -licm-pass example.ll -S -o example_out.ll
```

Run Loop Unrolling pass:
```bash
./bin/opt -enable-new-pm=0 -load lib/LLVMLoopUnrollingPass.so -loop-unrolling example.ll -S -o example_out.ll
```

Run both passes:
```bash
./bin/opt -enable-new-pm=0 \
    -load lib/LLVMLicmPass.so \
    -load lib/LLVMLoopUnrollingPass.so \
    -licm-pass -loop-unrolling example.ll -S -o example_out.ll
```
