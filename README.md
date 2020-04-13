# Inception-translator

The Inception Translator is a framework to lower low-level code into LLVM-IR.
In particular, it generates a unified LLVM-IR from a ARM32 binary and a LLVM bitcode generated from the source code.
This framework uses a lift-and-merge process that is built around two main components:
* A ARM32 lifter for translating ARM32 machine instructions to LLVM-IR.
* A merger that enables execution of IR generated from low-level semantic code (e.g., binary or inline assembly) and high-level semantic code (e.g., C/C++).

The lifter can be used independently of the Inception eco-system.

# How to build?

```bash
mkdir build
cd build
cmake ..
make
```
