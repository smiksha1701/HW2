CS7331 HW2: LLVM LibToolsThis repository contains two LLVM-based tools developed using Clang LibTooling.1. Code Analysis Tool (FunctionFinder)Analyzes C/C++ source files and extracts statistics for every function definition.Usage:./build/FunctionFinder
(Note: Uses compile_commands.json to find source files automatically)Features:Extracts Function NameCounts ArgumentsCounts Statements (excluding headers)Counts Loops (for, while, do-while)Counts Call Sites (within the same translation unit)2. Source-to-Source Transformation (StrengthReducer)Performs Operator Strength Reduction by replacing integer multiplication/division operations involving powers of two with bitwise shifts.Usage:./build/StrengthReducer <filename> -- <compiler-flags>
Example:./build/StrengthReducer test_transform.cpp -- -std=c++17 -stdlib=libstdc++
Transformations:x * 4 -> x << 2y / 8 -> y >> 3Ignored: Non-power-of-two operands (e.g., x * 3).Build InstructionsPrerequisites: LLVM 18, Clang 18, CMake.mkdir build
cd build
cmake ..
make
LimitationsFunctionFinder: Call counts are restricted to the single translation unit being analyzed.StrengthReducer: Only handles integer literals on the right-hand side. Does not handle complex expressions resolving to powers of two.
**2. Initialize Git and Submit:**

Run these commands in your `~/hw2` directory:

```bash
# Initialize repo
git init

# Add your files (Source, CMake, Readme, Tests)
git add FunctionFinder.cpp StrengthReducer.cpp CMakeLists.txt compile_commands.json test.cpp test_transform.cpp README.md

# Commit
git commit -m "Completed HW2: FunctionFinder and StrengthReducer"

# (Optional) Push to GitHub/GitLab if required by your professor
# git remote add origin <your-repo-url>
# git push -u origin main
