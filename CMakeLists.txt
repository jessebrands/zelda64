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
option(ZELDA64_BUILD_ZELDA64 "Enable building zelda64 program" ${PROJECT_IS_TOP_LEVEL})

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
        lib/crc32c.h
        lib/error.h
        lib/layout.h
        lib/rom.h
        lib/source.h)

set(LIBZELDA64_SOURCES
        lib/allocator.c
        lib/crc32c.c
        lib/error.c
        lib/layout_free.c
        lib/layout_operation.c
        lib/layout_write.c
        lib/rom_cic.c
        lib/rom_compress.c
        lib/rom_decompress.c
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

set(ZELDA64_HEADERS)

set(ZELDA64_SOURCES
        src/zelda64.c)

if (ZELDA64_BUILD_ZELDA64)
    add_executable(zelda64
            ${ZELDA64_SOURCES}
            ${ZELDA64_HEADERS})

    target_link_libraries(zelda64
            PRIVATE yaz0::yaz0
            PRIVATE zelda64::zelda64)

    target_compile_features(zelda64 PUBLIC c_std_99)
    target_compile_options(zelda64 PRIVATE ${ZELDA64_WARNING_FLAGS})
endif ()
