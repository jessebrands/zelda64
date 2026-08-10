/*
 * dma.c: DMA table reading and writing
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

#include <assert.h>
#include <stdbool.h>

#include "bytes.h"
#include "zelda64/zelda64.h"

static struct zelda64_dma_entry
read_entry_unsafe(uint8_t const* data) {
    return (struct zelda64_dma_entry){
        .vrom_start = zelda64_read_u32(&data[0]),
        .vrom_end = zelda64_read_u32(&data[4]),
        .rom_start = zelda64_read_u32(&data[8]),
        .rom_end = zelda64_read_u32(&data[12])
    };
}

static inline bool
can_be_makerom_entry(struct zelda64_dma_entry const entry) {
    return entry.vrom_start == 0 && entry.vrom_end != 0
           && entry.rom_start == 0 && entry.rom_end == 0;
}

enum zelda64_result
zelda64_find_dmadata_start(uint8_t const* data, size_t const size, size_t* offset) {
    if (data == NULL || offset == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    // The DMADATA starts with the entry for MAKEROM. We don't know how big the
    // MAKEROM really is, so we're going to get some false positives.
    while (*offset < size && size - *offset >= ZELDA64_DMA_ENTRY_SIZE) {
        struct zelda64_dma_entry const entry = read_entry_unsafe(&data[*offset]);
        if (can_be_makerom_entry(entry)) {
            return ZELDA64_OK;
        }

        *offset += ZELDA64_DMA_ENTRY_SIZE;
    }

    return ZELDA64_NO_DMADATA;
}

enum zelda64_result
zelda64_read_dmadata_info(struct zelda64_dmadata_info* info, size_t const offset, size_t const rom_size,
                          uint8_t const* data, size_t const size) {
    if (info == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_DMA_ENTRY_SIZE * 3) {
        return ZELDA64_OUT_OF_RANGE;
    }

    *info = (struct zelda64_dmadata_info){0};

    /*
     * In short, we're looking for a certain signature.
     *
     * let A = size of MAKEROM
     * let B = size of BOOTCODE
     * let C = size of DMADATA
     *
     *           vrom_start  vrom_end    rom_start   rom_end       filename
     *     0x00  0x00000000  A           0x00000000  0x00000000    MAKEROM
     *     0x01  A           A+B         A           0x00000000    BOOTCODE
     *     0x02  A+B         A+B+C       A+B         0x00000000    DMADATA
     *
     * DMADATA offset        =   A+B
     * DMADATA size in bytes =  (A+B+C) - (A+B)
     * DMADATA entry count   = ((A+B+C) - (A+B)) / 16
     */

    struct zelda64_dma_entry const e0 = read_entry_unsafe(&data[ZELDA64_DMA_ENTRY_SIZE * 0]);
    struct zelda64_dma_entry const e1 = read_entry_unsafe(&data[ZELDA64_DMA_ENTRY_SIZE * 1]);
    struct zelda64_dma_entry const e2 = read_entry_unsafe(&data[ZELDA64_DMA_ENTRY_SIZE * 2]);

    if (!can_be_makerom_entry(e0)) {
        return ZELDA64_NO_DMADATA;
    }

    bool const e1_is_bootcode = e1.vrom_start == e0.vrom_end
                                && e1.rom_start == e0.vrom_end
                                && e1.rom_end == 0;

    bool const e2_is_dmadata = e2.vrom_start == e1.vrom_end
                               && e2.rom_start == e1.vrom_end
                               && e2.rom_end == 0;

    if (!e1_is_bootcode || !e2_is_dmadata) {
        return ZELDA64_NO_DMADATA;
    }

    // The chain must be non-decreasing, and must fit inside the ROM.
    if (e1.vrom_end < e1.vrom_start || e2.vrom_end > rom_size) {
        return ZELDA64_NO_DMADATA;
    }

    // At this point we're already 99% certain, but just in case, just check
    // that this entry actually describes something that could be DMADATA.
    if (e2.vrom_end <= e2.vrom_start || e2.vrom_start != offset) {
        return ZELDA64_NO_DMADATA;
    }

    uint32_t const span = e2.vrom_end - e2.vrom_start;
    if (span % ZELDA64_DMA_ENTRY_SIZE != 0) {
        return ZELDA64_NO_DMADATA;
    }

    // As far as we can tell, this is DMADATA!
    *info = (struct zelda64_dmadata_info){
        .offset = e2.vrom_start,
        .size = span,
        .count = span / ZELDA64_DMA_ENTRY_SIZE,
    };

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_find_dmadata(struct zelda64_dmadata_info* info, uint8_t const* rom, size_t const rom_size) {
    if (info == NULL || rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    *info = (struct zelda64_dmadata_info){0};

    size_t dma_start = 0;
    enum zelda64_result result = ZELDA64_NO_DMADATA;

    while ((result = zelda64_find_dmadata_start(rom, rom_size, &dma_start)) == ZELDA64_OK) {
        uint8_t const* dmadata = &rom[dma_start];
        size_t const dmadata_size = rom_size - dma_start;

        result = zelda64_read_dmadata_info(info, dma_start, rom_size, dmadata, dmadata_size);
        if (result == ZELDA64_OK) {
            break;
        }

        dma_start += ZELDA64_DMA_ENTRY_SIZE;
    }

    return result;
}

enum zelda64_result
zelda64_read_dmadata(struct zelda64_dma_entry* entries, size_t const count,
                     uint8_t const* data, size_t const size) {
    if (entries == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (count > size / ZELDA64_DMA_ENTRY_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    for (size_t i = 0; i < count; ++i) {
        size_t const position = i * ZELDA64_DMA_ENTRY_SIZE;
        entries[i] = read_entry_unsafe(&data[position]);
    }

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_write_dmadata(uint8_t* data, size_t const size,
                      struct zelda64_dma_entry const* entries, size_t const count) {
    if (entries == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (count > size / ZELDA64_DMA_ENTRY_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    for (size_t i = 0; i < count; ++i) {
        size_t const position = i * ZELDA64_DMA_ENTRY_SIZE;
        struct zelda64_dma_entry const* entry = &entries[i];
        zelda64_write_u32(&data[position + 0], entry->vrom_start);
        zelda64_write_u32(&data[position + 4], entry->vrom_end);
        zelda64_write_u32(&data[position + 8], entry->rom_start);
        zelda64_write_u32(&data[position + 12], entry->rom_end);
    }

    return ZELDA64_OK;
}

enum zelda64_dma_kind zelda64_dma_entry_kind(struct zelda64_dma_entry const* entry) {
    assert(entry != NULL);

    if (entry->vrom_end == entry->vrom_start) {
        return ZELDA64_DMA_EMPTY;
    }
    if (entry->rom_end == UINT32_MAX) {
        return ZELDA64_DMA_DELETED;
    }
    if (entry->rom_end == 0) {
        return ZELDA64_DMA_UNCOMPRESSED;
    }
    return ZELDA64_DMA_COMPRESSED;
}

enum zelda64_result
zelda64_dma_entry_extent(struct zelda64_dma_entry const* entry,
                         uint32_t* offset, uint32_t* size) {
    if (entry == NULL || offset == NULL || size == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(entry);
    switch (kind) {
        case ZELDA64_DMA_EMPTY:
            *offset = entry->rom_start;
            *size = 0;
            return ZELDA64_OK;

        case ZELDA64_DMA_DELETED:
            return ZELDA64_INVALID_PARAMETER;

        case ZELDA64_DMA_UNCOMPRESSED:
            if (entry->vrom_start > entry->vrom_end) {
                return ZELDA64_OUT_OF_RANGE;
            }
            *offset = entry->rom_start;
            *size = entry->vrom_end - entry->vrom_start;
            return ZELDA64_OK;

        case ZELDA64_DMA_COMPRESSED:
            if (entry->rom_start > entry->rom_end) {
                return ZELDA64_OUT_OF_RANGE;
            }
            *offset = entry->rom_start;
            *size = entry->rom_end - entry->rom_start;
            return ZELDA64_OK;
    }

    return ZELDA64_INVALID_PARAMETER;
}
