/*
 * manifest.c: manifest file operations
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of zelda64.
 *
 * zelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * zelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zelda64/zmf.h>

#include "log.h"
#include "manifest.h"

static enum zelda64_result
create_manifest_file(char const* filename, struct zelda64_rom const* rom) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        return ZELDA64_IO_ERROR;
    }

    struct zelda64_zmf_header header = {
        .version = ZELDA64_ZMF_MAKE_VERSION(1, 0),
        .game_version = rom->info.header.version,
        .check_code = rom->info.header.check_code,
        .program = "libzelda64 " ZELDA64_VERSION_STRING,
    };

    memcpy(&header.game_code, rom->info.header.game_code, sizeof header.game_code);

    uint8_t buffer[ZELDA64_ZMF_HEADER_SIZE];

    enum zelda64_result const result = zelda64_zmf_write_header(buffer, sizeof buffer, &header);
    if (result != ZELDA64_OK) {
        fclose(file);
        return result;
    }

    size_t const bytes_out = fwrite(buffer, 1, sizeof buffer, file);
    fclose(file);

    return bytes_out == sizeof buffer
               ? ZELDA64_OK
               : ZELDA64_IO_ERROR;
}

static enum zelda64_result
zelda64_manifest_append_copy_list(char const* filename, struct zelda64_rom const* rom) {
    enum zelda64_result result = ZELDA64_OK;

    size_t const list_size = zelda64_zmf_copy_list_size(rom->dma_info.count);
    size_t const padded = (list_size + 15u) & (size_t) -16;

    uint8_t* list = calloc(padded, 1);
    if (list == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    for (size_t i = 0; i < rom->dma_info.count; ++i) {
        if (zelda64_dma_entry_kind(&rom->dma_table[i]) == ZELDA64_DMA_UNCOMPRESSED) {
            zelda64_zmf_copy_list_set(list, list_size, i);
        }
    }

    FILE* file = fopen(filename, "ab");
    if (file == NULL) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_list;
    }

    struct zelda64_zmf_chunk_header const header = {
        .type = {'Z', 'M', 'C', 'L'},
        .length = (uint32_t) list_size,
        .checksum = zelda64_zmf_chunk_checksum(0, list, list_size),
    };

    uint8_t chunk[0x10] = {0};
    result = zelda64_zmf_write_chunk_header(chunk, sizeof chunk, &header);
    if (result != ZELDA64_OK) {
        goto cleanup_file;
    }

    if (fwrite(chunk, sizeof chunk, 1, file) != 1) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_file;
    }

    if (fwrite(list, padded, 1, file) != 1) {
        result = ZELDA64_IO_ERROR;
    }

cleanup_file:
    fclose(file);
cleanup_list:
    free(list);
    return result;
}

enum zelda64_result
zelda64_make_rom_manifest(char const* filename, struct zelda64_rom const* rom) {
    if (filename == NULL || rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    enum zelda64_result const result = create_manifest_file(filename, rom);
    if (result != ZELDA64_OK) {
        return result;
    }

    return zelda64_manifest_append_copy_list(filename, rom);
}

enum zelda64_result
zelda64_read_rom_op_list(char const* filename, struct zelda64_rom const* rom,
                         uint8_t* ops, size_t const count) {
    return ZELDA64_BAD_MANIFEST;
}
