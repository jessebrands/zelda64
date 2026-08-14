cmake_minimum_required(VERSION 3.21)
project(libzelda64
        VERSION 0.1.0
        DESCRIPTION "Nintendo 64 Zelda ROM library and utilities"
        HOMEPAGE_URL "https://github.com/jessebrands/zelda64"
        LANGUAGES C)

#
# Project wide CMake settings
#
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#
# Library settings
#
option(ZELDA64_BUILD_SHARED "Enable building libzelda64 as shared library" ON)
option(ZELDA64_BUILD_UTILITIES "Enable building utilities" ${PROJECT_IS_TOP_LEVEL})

#
# Dependencies
#
find_package(yaz0 1.0 REQUIRED)

configure_file(
        include/zelda64/config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h
        @ONLY)

if (MSVC)
    set(ZELDA64_WARNING_FLAGS /W4)
else ()
    set(ZELDA64_WARNING_FLAGS
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion
            -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings
            -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
            -Wold-style-definition -Wredundant-decls
            -Wundef -Wvla -Wdouble-promotion -Wformat=2 -Wnull-dereference)
endif ()

set(LIBZELDA64_PUBLIC_HEADERS
        include/zelda64/zelda64.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h)

set(LIBZELDA64_PRIVATE_HEADERS
        lib/allocator.h
        lib/bytes.h
        lib/error.h
        lib/source.h
        lib/rom.h)

set(LIBZELDA64_SOURCES
        lib/allocator.c
        lib/error.c
        lib/rom_dma.c
        lib/rom_open.c
        lib/rom_read.c
        lib/rom_stat.c
        lib/source_file.c
        lib/version.c)

if (ZELDA64_BUILD_SHARED)
    add_library(libzelda64_shared SHARED
            ${LIBZELDA64_SOURCES}
            ${LIBZELDA64_PUBLIC_HEADERS}
            ${LIBZELDA64_PRIVATE_HEADERS})

    add_library(zelda64::zelda64 ALIAS libzelda64_shared)

    target_include_directories(libzelda64_shared
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")

    target_link_libraries(libzelda64_shared
            PRIVATE yaz0::yaz0)

    set_target_properties(libzelda64_shared PROPERTIES
            OUTPUT_NAME "zelda64"
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

    target_compile_definitions(libzelda64_shared PRIVATE ZELDA64_SHARED)

    set_target_properties(libzelda64_shared PROPERTIES C_VISIBILITY_PRESET hidden)
    target_compile_features(libzelda64_shared PUBLIC c_std_99)
    target_compile_options(libzelda64_shared PRIVATE ${ZELDA64_WARNING_FLAGS})
endif ()

if (ZELDA64_BUILD_UTILITIES AND ZELDA64_BUILD_SHARED)
    add_executable(decompress
            src/decompress.c)

    target_link_libraries(decompress
            PRIVATE zelda64::zelda64)

    target_compile_features(decompress PRIVATE c_std_99)
    target_compile_options(decompress PRIVATE ${ZELDA64_WARNING_FLAGS})
endif ()
