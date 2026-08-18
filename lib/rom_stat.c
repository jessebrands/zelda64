/*
 * rom_start.c: get information about a file in a ROM
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libzelda64.
 *
 * libzelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libzelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libzelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <yaz0/yaz0.h>

#include "rom.h"
#include "zelda64/zelda64.h"

enum zelda64_dmadata_kind
zelda64_dmadata_kind(struct zelda64_dmadata const* entry) {
    assert(entry != NULL);
    if (entry->rom_end == UINT32_MAX) {
        return ZELDA64_ENTRY_DELETED;
    }
    if (entry->rom_end == 0) {
        return ZELDA64_ENTRY_STORED;
    }
    return ZELDA64_ENTRY_COMPRESSED;
}

static enum zelda64_result
zelda64_dma_entry_extent(struct zelda64_dmadata const* entry,
                         uint32_t* offset, uint32_t* size,
                         struct zelda64_error* error) {
    if (entry == NULL || offset == NULL || size == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }

    enum zelda64_dmadata_kind const kind = zelda64_dmadata_kind(entry);
    switch (kind) {
        case ZELDA64_ENTRY_DELETED: {
            return zelda64_set_error(error, ZELDA64_DELETED);
        }

        case ZELDA64_ENTRY_STORED: {
            if (entry->vrom_start > entry->vrom_end) {
                return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
            }
            *offset = entry->rom_start;
            *size = entry->vrom_end - entry->vrom_start;
            return ZELDA64_OK;
        }

        case ZELDA64_ENTRY_COMPRESSED: {
            if (entry->vrom_start > entry->vrom_end) {
                return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
            }
            if (entry->rom_start > entry->rom_end) {
                return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
            }
            *offset = entry->rom_start;
            *size = entry->rom_end - entry->rom_start;
            return ZELDA64_OK;
        }
    }

    return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
}

enum zelda64_result
zelda64_stat(struct zelda64_stat* st,
             struct zelda64_rom const* rom, zelda64_index_t const index,
             struct zelda64_error* error) {
    if (st == NULL || rom == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }
    if (index >= rom->dmadata_info.count) {
        return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
    }

    struct zelda64_dmadata const* entry = &rom->dmadata[index];
    enum zelda64_dmadata_kind const kind = zelda64_dmadata_kind(entry);

    uint32_t offset = 0;
    uint32_t size = 0;
    if (zelda64_dma_entry_extent(entry, &offset, &size, error) != ZELDA64_OK) {
        return error->result;
    }

    struct zelda64_stat stat = {
        .method = ZELDA64_METHOD_STORE,
        .offset = offset,
        .size = size,
        .file_size = entry->vrom_end - entry->vrom_start,
    };

    // If this file is a compressed file, we can get the real size from the
    // Yaz0 file header instead.
    if (kind == ZELDA64_ENTRY_COMPRESSED) {
        stat.method = ZELDA64_METHOD_YAZ0;

        // A compressed file is always at least 16 bytes in the ROM, as it
        // must contain the header.
        if (stat.size < 16) {
            return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        }

        // Sadly, libyaz0 forgot to export the header size in bytes.
        // But a header struct is always at least as big!
        uint8_t header_bytes[16];
        zelda64_io_read_exact(&rom->io, header_bytes, sizeof header_bytes, offset, error);
        if (ZELDA64_FAILED(error)) {
            return error->result;
        }

        // Now we can read the header.
        struct yaz0_header header = {0};
        enum yaz0_result const result = yaz0_read_header(header_bytes, sizeof header_bytes, &header);
        if (result != YAZ0_OK) {
            return zelda64_set_sys_error(error, ZELDA64_DECOMPRESS_ERROR, result);
        }

        stat.file_size = header.uncompressed_size;
    }

    *st = stat;
    return ZELDA64_OK;
}
