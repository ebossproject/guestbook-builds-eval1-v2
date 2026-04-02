cmake_minimum_required(VERSION 3.20)
project(guestbook VERSION 0.10.2 LANGUAGES CXX)

# Configure vcpkg toolchain file if it hasn't been configured elsewhere
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Try to find vcpkg from environment variable
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    else()
        message(WARNING "VCPKG_ROOT environment variable not defined. Please define it or provide CMAKE_TOOLCHAIN_FILE manually.")
    endif()
endif()

# The tests share this list of source files
set(guestbook_srcs
    src/assert.cpp
    src/base64.cpp
    src/db.cpp
    src/session.cpp
    src/template_finder.cpp
    src/util_rand.cpp
    src/tpl/filter.cpp
    src/tpl/index.cpp
    src/tpl/info.cpp
)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Main executable - using the target name "guestbook" to match build script
add_executable(guestbook
    src/main.cpp
    ${guestbook_srcs}
)

# Use C++20
target_compile_features(guestbook PRIVATE cxx_std_20)

# Include directories
target_include_directories(
    guestbook PRIVATE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

# ---- Dependencies ----

# Find Crow and asio (from build log)
find_package(Crow CONFIG REQUIRED)
target_link_libraries(guestbook PRIVATE Crow::Crow asio::asio)

# Find SQLite3
find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(guestbook PRIVATE unofficial::sqlite3::sqlite3)

# Find libsodium
find_package(unofficial-sodium CONFIG REQUIRED)
target_link_libraries(guestbook PRIVATE unofficial-sodium::sodium)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(GNUInstallDirs)
    install(
        TARGETS guestbook
        RUNTIME COMPONENT guestbook_Runtime
    )
    
    # Install views directory to share/guestbook
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/views/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/guestbook
        COMPONENT guestbook_Runtime
    )
endif()