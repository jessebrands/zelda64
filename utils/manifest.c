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
read_payload(struct zelda64_manifest const* manifest,
             struct zelda64_zmf_chunk_header const* chunk, size_t const offset,
             uint8_t** data, size_t* size) {
    if (chunk->length == 0) {
        return ZELDA64_BAD_MANIFEST;
    }

    uint8_t* buffer = malloc(chunk->length);
    if (buffer == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    if (fseek(manifest->file, (long) offset, SEEK_SET) != 0
        || fread(buffer, 1, chunk->length, manifest->file) != chunk->length) {
        free(buffer);
        return ZELDA64_BAD_MANIFEST;
    }

    if (chunk->checksum != 0
        && zelda64_zmf_chunk_checksum(0, buffer, chunk->length) != chunk->checksum) {
        free(buffer);
        return ZELDA64_BAD_MANIFEST;
    }

    *data = buffer;
    *size = chunk->length;
    return ZELDA64_OK;
}

static enum zelda64_result
read_manifest_header(struct zelda64_manifest* manifest) {
    uint8_t buffer[ZELDA64_ZMF_HEADER_SIZE];
    if (fread(buffer, sizeof buffer, 1, manifest->file) != 1) {
        return ZELDA64_BAD_MANIFEST;
    }

    enum zelda64_result const result = zelda64_zmf_read_header(&manifest->header, buffer, sizeof buffer);
    if (result != ZELDA64_OK) {
        return result;
    }
    if (ZELDA64_ZMF_VERSION_MAJOR(manifest->header.version) > ZELDA64_ZMF_VERSION_MAJOR(ZELDA64_ZMF_VERSION)) {
        return ZELDA64_UNSUPPORTED_VERSION;
    }

    return ZELDA64_OK;
}

static enum zelda64_result
read_manifest_chunks(struct zelda64_manifest* manifest) {
    enum zelda64_result result = ZELDA64_OK;
    size_t position = ZELDA64_ZMF_HEADER_SIZE;

    while (position < manifest->file_size) {
        uint8_t bytes[ZELDA64_ZMF_CHUNK_HEADER_SIZE];
        if (fseek(manifest->file, (long) position, SEEK_SET) != 0
            || fread(bytes, sizeof bytes, 1, manifest->file) != 1) {
            return ZELDA64_BAD_MANIFEST;
        }

        struct zelda64_zmf_chunk_header chunk;
        result = zelda64_zmf_read_chunk_header(&chunk, bytes, sizeof bytes);
        if (result != ZELDA64_OK) {
            return result;
        }

        size_t payload = 0;
        size_t next = 0;
        result = zelda64_zmf_chunk_extent(&chunk, position, manifest->file_size, &payload, &next);
        if (result != ZELDA64_OK) {
            return result;
        }

        if (manifest->copy_list == NULL
            && memcmp(chunk.type, ZELDA64_ZMF_TYPE_COPY_LIST, sizeof chunk.type) == 0) {
            result = read_payload(manifest, &chunk, payload, &manifest->copy_list, &manifest->copy_list_size);
            if (result != ZELDA64_OK) {
                return result;
            }
        }

        position = next;
    }

    return result;
}

enum zelda64_result
zelda64_open_manifest(char const* filename, struct zelda64_manifest* manifest) {
    enum zelda64_result result = ZELDA64_OK;
    if (filename == NULL || manifest == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    *manifest = (struct zelda64_manifest){0};
    manifest->filename = filename;

    manifest->file = fopen(filename, "rb");
    if (manifest->file == NULL) {
        return ZELDA64_IO_ERROR;
    }

    if (fseek(manifest->file, 0, SEEK_END) != 0) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_file;
    }
    long const size = ftell(manifest->file);
    if (size < 0) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_file;
    }

    manifest->file_size = (size_t) size;
    rewind(manifest->file);

    result = read_manifest_header(manifest);
    if (result != ZELDA64_OK) {
        goto cleanup_file;
    }

    result = read_manifest_chunks(manifest);
    if (result != ZELDA64_OK) {
        goto cleanup_file;
    }

    return ZELDA64_OK;

cleanup_file:
    fclose(manifest->file);
    return result;
}

void
zelda64_close_manifest(struct zelda64_manifest* manifest) {
    if (manifest == NULL) {
        return;
    }

    free(manifest->copy_list);
    manifest->copy_list = NULL;

    if (manifest->file != NULL) {
        fclose(manifest->file);
        manifest->file = NULL;
    }
}

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
zelda64_manifest_check_input_rom(struct zelda64_manifest const* manifest,
                                 struct zelda64_rom const* rom) {
    if (manifest == NULL || rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (manifest->copy_list_size != zelda64_zmf_copy_list_size(rom->dma_info.count)) {
        return ZELDA64_BAD_MANIFEST;
    }

    log_info("Manifest matches input ROM");
    return ZELDA64_OK;
}

enum zelda64_result
zelda64_manifest_check_output_rom(struct zelda64_manifest const* manifest,
                                  struct zelda64_rom const* rom) {
    if (manifest == NULL || rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    log_info("Verifying output ROM against manifest");
    if (manifest->header.check_code != rom->info.header.check_code) {
        if (manifest->header.check_code != 0) {
            log_error("Output ROM check code does not match manifest!");
            logf_error("Check code is %016" PRIX64 ", expected %016" PRIX64,
                       rom->info.header.check_code,
                       manifest->header.check_code);

            return ZELDA64_MISMATCH;
        }

        log_info("Manifest carries no check code, skipping check code verification");
    }

    log_info("Output ROM matches manifest, verification OK");
    return ZELDA64_OK;
}

enum zelda64_operation
zelda64_manifest_operation(struct zelda64_manifest const* manifest,
                           struct zelda64_dma_entry const* entry, size_t const index) {
    enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(entry);
    if (kind == ZELDA64_DMA_EMPTY || kind == ZELDA64_DMA_DELETED) {
        return ZELDA64_OPERATION_PASS;
    }

    return zelda64_zmf_copy_list_test(manifest->copy_list, manifest->copy_list_size, index)
               ? ZELDA64_OPERATION_COPY
               : ZELDA64_OPERATION_COMPRESS;
}
