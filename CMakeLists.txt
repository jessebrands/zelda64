cmake_minimum_required(VERSION 3.21...3.31)
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
option(ZELDA64_BUILD_STATIC "Enable building libzelda64 as static library" ON)
option(ZELDA64_BUILD_UTILITIES "Enable building libzelda64-utils" ${PROJECT_IS_TOP_LEVEL})
option(ZELDA64_BUILD_TESTS "Enable building tests" ${PROJECT_IS_TOP_LEVEL})
option(ZELDA64_INSTALL "Enable installation of libzelda64 and libzelda64-utils" ${PROJECT_IS_TOP_LEVEL})

set(ZELDA64_PUBLIC_HEADERS
        include/zelda64/zelda64.h
        include/zelda64/zmf.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h)

set(ZELDA64_PRIVATE_HEADERS
        src/bytes.h
        src/crc32.h
        src/rom.h)

set(ZELDA64_SOURCES
        src/crc32.c
        src/dma.c
        src/rom.c
        src/zelda64.c
        src/zmf.c)

function(zelda64_enable_warnings target)
    if (MSVC)
        target_compile_options(${target}
                PRIVATE /W4
                PRIVATE /w14242    # -Wconversion
                PRIVATE /w14254    # narrower bit-field conversion
                PRIVATE /w14255    # -Wstrict-prototypes
                PRIVATE /w14296    # unsigned compares
                PRIVATE /w14311    # pointer truncation
                PRIVATE /w14826)   # sign-extended conversion

        # Suppress 'warnings' that we're not using MSVC's annoying extensions
        target_compile_definitions(${target}
                PRIVATE _CRT_SECURE_NO_WARNINGS)
    else ()
        target_compile_options(${target}
                PRIVATE -Wall -Wextra -pedantic
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes -Wmissing-prototypes
                PRIVATE -Wold-style-definition)
    endif ()
endfunction()

configure_file(
        include/zelda64/config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h
        @ONLY)

if (ZELDA64_BUILD_SHARED)
    add_library(zelda64_shared SHARED
            ${ZELDA64_SOURCES}
            ${ZELDA64_PUBLIC_HEADERS}
            ${ZELDA64_PRIVATE_HEADERS})

    add_library(zelda64::zelda64 ALIAS zelda64_shared)

    target_include_directories(zelda64_shared
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    target_compile_definitions(zelda64_shared PRIVATE ZELDA64_SHARED)

    target_compile_features(zelda64_shared PUBLIC c_std_99)
    set_target_properties(zelda64_shared PROPERTIES
            EXPORT_NAME "zelda64"
            OUTPUT_NAME "zelda64"
            DEBUG_POSTFIX "d"
            C_VISIBILITY_PRESET hidden
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

    zelda64_enable_warnings(zelda64_shared)
endif ()

if (ZELDA64_BUILD_STATIC OR ZELDA64_BUILD_TESTS OR ZELDA64_BUILD_UTILITIES)
    add_library(zelda64_static STATIC
            ${ZELDA64_SOURCES}
            ${ZELDA64_PUBLIC_HEADERS}
            ${ZELDA64_PRIVATE_HEADERS})

    add_library(zelda64::zelda64_static ALIAS zelda64_static)

    target_include_directories(zelda64_static
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    target_compile_features(zelda64_static PUBLIC c_std_99)
    target_compile_definitions(zelda64_static PUBLIC ZELDA64_STATIC)

    if (WIN32)
        set_target_properties(zelda64_static PROPERTIES OUTPUT_NAME "zelda64s")
    else ()
        set_target_properties(zelda64_static PROPERTIES OUTPUT_NAME "zelda64")
    endif ()

    set_target_properties(zelda64_static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            DEBUG_POSTFIX "d")

    zelda64_enable_warnings(zelda64_static)
endif ()

if (ZELDA64_BUILD_UTILITIES OR ZELDA64_BUILD_TESTS)
    add_subdirectory("utils")
endif ()

if (ZELDA64_BUILD_TESTS)
    enable_testing()
    add_subdirectory("test")
endif ()

set(ZELDA64_INSTALL_LIBRARY_TARGETS)
set(ZELDA64_INSTALL_PROGRAM_TARGETS)

if (ZELDA64_BUILD_SHARED)
    list(APPEND ZELDA64_INSTALL_LIBRARY_TARGETS zelda64_shared)
endif ()
if (ZELDA64_BUILD_STATIC)
    list(APPEND ZELDA64_INSTALL_LIBRARY_TARGETS zelda64_static)
endif ()
if (ZELDA64_BUILD_UTILITIES)
    list(APPEND ZELDA64_INSTALL_PROGRAM_TARGETS
            compress
            decompress)
endif ()

if (ZELDA64_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    if (ZELDA64_INSTALL_LIBRARY_TARGETS)
        install(TARGETS ${ZELDA64_INSTALL_LIBRARY_TARGETS}
                EXPORT zelda64Targets
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

        install(FILES ${ZELDA64_PUBLIC_HEADERS}
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zelda64")

        install(EXPORT zelda64Targets
                FILE zelda64Targets.cmake
                NAMESPACE zelda64::
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zelda64")

        configure_package_config_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/zelda64Config.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/zelda64Config.cmake"
                INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zelda64")

        write_basic_package_version_file(
                "${CMAKE_CURRENT_BINARY_DIR}/zelda64ConfigVersion.cmake"
                VERSION ${PROJECT_VERSION}
                COMPATIBILITY SameMinorVersion)

        install(FILES
                "${CMAKE_CURRENT_BINARY_DIR}/zelda64Config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/zelda64ConfigVersion.cmake"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zelda64")

        install(FILES COPYING COPYING.LESSER
                DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/libzelda64")
    endif ()


    if (ZELDA64_INSTALL_PROGRAM_TARGETS)
        foreach (program IN LISTS ZELDA64_INSTALL_PROGRAM_TARGETS)
            install(PROGRAMS "$<TARGET_FILE:${program}>"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}"
                    RENAME "zelda64-${program}${CMAKE_EXECUTABLE_SUFFIX}")
        endforeach ()

        install(FILES COPYING
                DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/libzelda64-utils")
    endif ()

    if (ZELDA64_INSTALL_LIBRARY_TARGETS OR ZELDA64_INSTALL_PROGRAM_TARGETS)
        install(FILES doc/MANIFEST README NEWS DESTINATION "${CMAKE_INSTALL_DOCDIR}")
    endif ()
endif ()
