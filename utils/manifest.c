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

#include <stdio.h>
#include <string.h>

#include "manifest.h"

#include "zelda64/zmf.h"

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
zelda64_manifest_append_op_list(char const* filename, struct zelda64_rom const* rom) {
    enum zelda64_result result = ZELDA64_OK;

    FILE* file = fopen(filename, "ab");
    if (file == NULL) {
        return ZELDA64_IO_ERROR;
    }

    {
        struct zelda64_zmf_chunk_header const header = {
            .type = {'Z', 'M', 'O', 'P'},
            .length = (uint32_t) rom->dma_info.count,
            .checksum = 0,
            .reserved = {0}
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
    }

    size_t entries_out = 0;
    for (size_t i = 0; i < rom->dma_info.count; i += 0x10) {
        uint8_t chunk[0x10] = {0};
        size_t const remaining = rom->dma_info.count - entries_out;
        size_t const count = remaining < sizeof chunk ? remaining : sizeof chunk;

        for (size_t j = 0; j < count; ++j) {
            struct zelda64_dma_entry const* entry = &rom->dma_table[i + j];
            enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(entry);
            chunk[j] = (uint8_t) zelda64_dma_kind_to_op(kind);
        }

        if (fwrite(chunk, sizeof chunk, 1, file) != 1) {
            result = ZELDA64_IO_ERROR;
            goto cleanup_file;
        }

        entries_out += count;
    }

cleanup_file:
    fclose(file);
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

    return zelda64_manifest_append_op_list(filename, rom);
}
