# CS7331 HW2: LLVM LibTools

This repository contains two LLVM-based tools developed using Clang LibTooling.

## 1. Code Analysis Tool (`FunctionFinder`)

Analyzes C/C++ source files and extracts statistics for every function definition.

**Usage:**

```bash
./build/FunctionFinder
```

*(Note: Uses `compile_commands.json` to find source files automatically)*

**Features:**

* Extracts Function Name
* Counts Arguments
* Counts Statements (excluding headers)
* Counts Loops (`for`, `while`, `do-while`)
* Counts Call Sites (within the same translation unit)

## 2. Source-to-Source Transformation (`StrengthReducer`)

Performs Operator Strength Reduction by replacing integer multiplication/division operations involving powers of two with bitwise shifts.

**Usage:**

```bash
./build/StrengthReducer <filename> -- <compiler-flags>
```

**Example:**

```bash
./build/StrengthReducer test_transform.cpp -- -std=c++17 -stdlib=libstdc++
```

**Transformations:**

* `x * 4` -> `x << 2`
* `y / 8` -> `y >> 3`
* Ignored: Non-power-of-two operands (e.g., `x * 3`).

## Build Instructions

Prerequisites: LLVM 18, Clang 18, CMake.

```bash
mkdir build
cd build
cmake ..
make
```

## Limitations

* `FunctionFinder`: Call counts are restricted to the single translation unit being analyzed.
* `StrengthReducer`: Only handles integer literals on the right-hand side. Does not handle complex expressions resolving to powers of two.
