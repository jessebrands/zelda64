function(zelda64_add_decompress_test input_rom check_rom)
    get_filename_component(base_name "${input_rom}" NAME_WLE)
    set(out_rom "${CMAKE_CURRENT_BINARY_DIR}/${base_name}-decompressed.z64")

    add_test(NAME "Decompress.${base_name}.Clean"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${out_rom}")
    set_tests_properties("Decompress.${base_name}.Clean" PROPERTIES
            FIXTURES_SETUP "decompressed_rom_${base_name}")

    add_test(NAME "Decompress.${base_name}"
            COMMAND decompress -v "${input_rom}" "${out_rom}")
    set_tests_properties("Decompress.${base_name}" PROPERTIES
            FIXTURES_SETUP "decompressed_rom_${base_name}"
            DEPENDS "Decompress.${base_name}.Clean"
            TIMEOUT 60)

    add_test(NAME "Decompress.${base_name}.Verify"
            COMMAND ${CMAKE_COMMAND} -E compare_files
            "${out_rom}" "${check_rom}")
    set_tests_properties("Decompress.${base_name}.Verify" PROPERTIES
            FIXTURES_REQUIRED "decompressed_rom_${base_name}")

endfunction()

function(zelda64_add_compress_test input_rom check_rom)
    get_filename_component(base_name "${input_rom}" NAME_WLE)
    set(out_rom "${CMAKE_CURRENT_BINARY_DIR}/${base_name}-compressed.z64")

    add_test(NAME "Compress.${base_name}.Clean"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${out_rom}")
    set_tests_properties("Compress.${base_name}.Clean" PROPERTIES
            FIXTURES_SETUP "compressed_rom_${base_name}")

    add_test(NAME "Compress.${base_name}"
            COMMAND compress -vv "${input_rom}" "${out_rom}")
    set_tests_properties("Compress.${base_name}" PROPERTIES
            FIXTURES_SETUP "compressed_rom_${base_name}"
            DEPENDS "Compress.${base_name}.Clean"
            TIMEOUT 60)

    add_test(NAME "Compress.${base_name}.Verify"
            COMMAND ${CMAKE_COMMAND} -E compare_files
            "${out_rom}" "${check_rom}")
    set_tests_properties("Compress.${base_name}.Verify" PROPERTIES
            FIXTURES_REQUIRED "compressed_rom_${base_name}")

endfunction()

set(ZELDA64_COMPRESSED_TEST_ROM "" CACHE FILEPATH "Official ROM image i.e. Ocarina of Time 1.0 US")
set(ZELDA64_DECOMPRESSED_TEST_ROM "" CACHE FILEPATH "Decompressed test ROM produced by a known good decompressor")

if (ZELDA64_COMPRESSED_TEST_ROM AND ZELDA64_DECOMPRESSED_TEST_ROM)
    zelda64_add_decompress_test(
            "${ZELDA64_COMPRESSED_TEST_ROM}"
            "${ZELDA64_DECOMPRESSED_TEST_ROM}")

    zelda64_add_compress_test(
            "${ZELDA64_DECOMPRESSED_TEST_ROM}"
            "${ZELDA64_COMPRESSED_TEST_ROM}")
else ()
    add_test(NAME "Decompress.Skipped" COMMAND ${CMAKE_COMMAND} -E echo
            "no test ROMs configured: set ZELDA64_COMPRESSED_TEST_ROM and ZELDA64_DECOMPRESSED_TEST_ROM")
    set_tests_properties("Decompress.Skipped" PROPERTIES
            SKIP_REGULAR_EXPRESSION "no test ROMs configured")
endif ()
