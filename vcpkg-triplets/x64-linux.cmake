# Copyright (c) 2026 Cromulence
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Path to our custom toolchain file (relative to the vcpkg root)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE /opt/toolchains/toolchain.cmake)

# Set build type
set(VCPKG_BUILD_TYPE release)
