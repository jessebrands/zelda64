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

#include "rom.h"
#include "zelda64/zelda64.h"

static enum zelda64_entry_kind
zelda64_dma_entry_kind(struct zelda64_dmadata const* entry) {
    assert(entry != NULL);

    if (entry->vrom_end == entry->vrom_start) {
        return ZELDA64_ENTRY_EMPTY;
    }
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

    enum zelda64_entry_kind const kind = zelda64_dma_entry_kind(entry);
    switch (kind) {
        case ZELDA64_ENTRY_EMPTY: {
            *offset = entry->rom_start;
            *size = 0;
            return ZELDA64_OK;
        }

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
             struct zelda64_rom const* rom, size_t index,
             struct zelda64_error* error) {
    if (st == NULL || rom == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }
    if (index >= rom->dmadata_info.count) {
        return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
    }

    struct zelda64_dmadata const* entry = &rom->dmadata[index];
    enum zelda64_entry_kind const kind = zelda64_dma_entry_kind(entry);

    uint32_t offset = 0;
    uint32_t size = 0;

    struct zelda64_error extent_error = {0};
    zelda64_dma_entry_extent(entry, &offset, &size, &extent_error);
    if (extent_error.result != ZELDA64_OK && extent_error.result != ZELDA64_DELETED) {
        *error = extent_error;
        return error->result;
    }

    *st = (struct zelda64_stat){
        .vrom_start = entry->vrom_start,
        .vrom_end = entry->vrom_end,
        .offset = offset,
        .size = size,
        .kind = kind,
    };

    return ZELDA64_OK;
}
