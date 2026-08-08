find_package(yaz0 REQUIRED)

add_executable(decompress
        decompress.c
        log.c
        rom.c)

target_link_libraries(decompress
        PRIVATE zelda64::zelda64_static
        PRIVATE yaz0::yaz0_static)

zelda64_enable_warnings(decompress)

add_executable(compress
        compress.c
        log.c
        rom.c)

target_link_libraries(compress
        PRIVATE zelda64::zelda64_static
        PRIVATE yaz0::yaz0_static)

zelda64_enable_warnings(compress)
