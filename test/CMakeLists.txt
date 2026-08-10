function(zelda64_add_test input_rom)
    get_filename_component(base_name "${input_rom}" NAME_WLE)
    set(decompressed_rom "${CMAKE_CURRENT_BINARY_DIR}/${base_name}-decompressed.z64")
    set(compressed_rom "${CMAKE_CURRENT_BINARY_DIR}/${base_name}-compressed.z64")
    set(manifest "${CMAKE_CURRENT_BINARY_DIR}/${base_name}.zmf")

    #
    # Destroy ROMs from a previous run.
    #
    add_test(NAME "${base_name}.Clean"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${decompressed_rom}" "${compressed_rom}" "${manifest}")
    set_tests_properties("${base_name}.Clean" PROPERTIES
            FIXTURES_SETUP "${base_name}")

    #
    # Decompress the input ROM.
    #
    add_test(NAME "${base_name}.Decompress"
            COMMAND decompress -v --pad --fill ramp
            --manifest "${manifest}"
            "${input_rom}" "${decompressed_rom}")
    set_tests_properties("${base_name}.Decompress" PROPERTIES
            FIXTURES_SETUP "${base_name}"
            DEPENDS "${base_name}.Clean"
            TIMEOUT 60)

    #
    # And compress it again.
    #
    add_test(NAME "${base_name}.Compress"
            COMMAND compress -v --pad --fill ramp
            --manifest "${manifest}"
            "${decompressed_rom}" "${compressed_rom}")
    set_tests_properties("${base_name}.Compress" PROPERTIES
            FIXTURES_SETUP "${base_name}"
            DEPENDS "${base_name}.Decompress"
            TIMEOUT 120)

    #
    # Verify it matches the retail ROM.
    #
    add_test(NAME "${base_name}.Verify"
            COMMAND ${CMAKE_COMMAND} -E compare_files
            "${compressed_rom}" "${input_rom}")
    set_tests_properties("${base_name}.Verify" PROPERTIES
            FIXTURES_REQUIRED "${base_name}")
endfunction()

set(ZELDA64_TEST_ROM "" CACHE FILEPATH "Official ROM image i.e. Ocarina of Time 1.0 US")

if (ZELDA64_TEST_ROM)
    zelda64_add_test("${ZELDA64_TEST_ROM}")
endif ()
