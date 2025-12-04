cmake_minimum_required(VERSION 3.10)

project(LLVMTools LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Make sure we use LLVM 18 & Clang 18
# (Change these paths if llvm-config --cmakedir shows something else)
set(LLVM_DIR "/usr/lib/llvm-18/lib/cmake/llvm")
set(Clang_DIR "/usr/lib/cmake/clang-18")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# ---- Find the LLVM shared library explicitly ----
find_library(LLVM_DYLIB
  NAMES LLVM-18
  HINTS
    ${LLVM_LIBRARY_DIRS}
    /usr/lib/llvm-18/lib
    /lib/x86_64-linux-gnu
)

if(NOT LLVM_DYLIB)
  message(FATAL_ERROR "Could not find the LLVM shared library (libLLVM-18.so)")
endif()

message(STATUS "Using LLVM shared library: ${LLVM_DYLIB}")

# --- Target 1: FunctionFinder ---
add_executable(FunctionFinder FunctionFinder.cpp)

target_link_libraries(FunctionFinder
  PRIVATE
  ${LLVM_DYLIB}
  clang-cpp
)

# --- Target 2: StrengthReducer ---
add_executable(StrengthReducer StrengthReducer.cpp)

target_link_libraries(StrengthReducer
  PRIVATE
  ${LLVM_DYLIB}
  clang-cpp
)
