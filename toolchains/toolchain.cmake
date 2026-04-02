# Copyright (c) 2026 Cromulence
# Use INIT variants in a toolchain file
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_LINKER ld.lld)

set(CMAKE_C_FLAGS_INIT "-DHELLO_THERE")
set(CMAKE_CXX_FLAGS_INIT "-DHELLO_THERE")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")

message(STATUS "Using custom Clang toolchain")