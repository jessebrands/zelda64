/*
 * rom_open.c: DMADATA functions
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
#include <stdint.h>

#include "bytes.h"
#include "rom.h"
#include "source.h"

#define CHUNK_SIZE 1024
#define READ_COUNT 128

static struct zelda64_dmadata
read_entry_unsafe(uint8_t const* data) {
    assert(data != NULL);

    return (struct zelda64_dmadata){
        .vrom_start = zelda64_read_u32(&data[0]),
        .vrom_end = zelda64_read_u32(&data[4]),
        .rom_start = zelda64_read_u32(&data[8]),
        .rom_end = zelda64_read_u32(&data[12])
    };
}

static bool
can_be_makerom(struct zelda64_dmadata const entry) {
    return entry.vrom_start == 0 && entry.vrom_end != 0
           && entry.rom_start == 0 && entry.rom_end == 0;
}

static bool
is_dmadata(struct zelda64_dmadata const e0,
           struct zelda64_dmadata const e1,
           struct zelda64_dmadata const e2,
           uint32_t const candidate, uint32_t const rom_size) {
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

    bool const e1_is_bootcode = e1.vrom_start == e0.vrom_end
                                && e1.rom_start == e0.vrom_end
                                && e1.rom_end == 0;

    bool const e2_is_dmadata = e2.vrom_start == e1.vrom_end
                               && e2.rom_start == e1.vrom_end
                               && e2.rom_end == 0;

    if (!e1_is_bootcode || !e2_is_dmadata) {
        return false;
    }

    // The chain must be non-decreasing, and must fit inside the ROM.
    if (e1.vrom_end < e1.vrom_start || e2.vrom_end > rom_size) {
        return false;
    }

    // At this point we're already 99% certain, but just in case, just check
    // that this entry actually describes something that could be DMADATA.
    if (e2.vrom_end <= e2.vrom_start || e2.vrom_start != candidate) {
        return false;
    }

    uint32_t const span = e2.vrom_end - e2.vrom_start;
    if (span % ZELDA64_DMA_ENTRY_SIZE != 0) {
        return false;
    }

    return true;
}

static enum zelda64_result
zelda64_find_dmadata(struct zelda64_dmadata_info* info,
                     struct zelda64_source const* source,
                     struct zelda64_error* error) {
    assert(info != NULL);
    assert(source != NULL);
    assert(error != NULL);

    // Get the ROM size and check its size.
    zelda64_ssize_t const file_size = zelda64_source_size(source, error);
    if (file_size < 0) {
        return error->result;
    }
    if (file_size > UINT32_MAX) {
        return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
    }

    uint32_t const rom_size = (uint32_t) file_size;
    uint32_t offset = 0;
    uint8_t chunk[CHUNK_SIZE];

    while (offset < rom_size) {
        uint32_t const remaining = rom_size - offset;
        uint32_t const want = remaining < sizeof chunk ? remaining : sizeof chunk;

        // Read in the next chunk of data.
        if (zelda64_source_read_exact(source, chunk, want, offset, error) < 0) {
            return error->result;
        }

        // Check every 16 bytes for potential MAKEROM entries.
        for (uint32_t i = 0; i + ZELDA64_DMA_ENTRY_SIZE <= want; i += ZELDA64_DMA_ENTRY_SIZE) {
            struct zelda64_dmadata const e0 = read_entry_unsafe(&chunk[i]);
            if (!can_be_makerom(e0)) {
                continue;
            }

            // This could be the MAKEROM! Get the next two entries and confirm.
            uint32_t const candidate = offset + i;
            uint8_t entry_buffer[ZELDA64_DMA_ENTRY_SIZE * 2];
            zelda64_ssize_t const bytes_in = zelda64_source_read_exact(
                source,
                entry_buffer, sizeof entry_buffer,
                candidate + ZELDA64_DMA_ENTRY_SIZE,
                error
            );

            if (bytes_in < 0) {
                // We're too close to EOF to be the file,
                // just let the function fail gracefully from here.
                if (error->result == ZELDA64_TRUNCATED) {
                    continue;
                }
                return error->result;
            }

            struct zelda64_dmadata const e1 = read_entry_unsafe(&entry_buffer[0]);
            struct zelda64_dmadata const e2 = read_entry_unsafe(&entry_buffer[ZELDA64_DMA_ENTRY_SIZE]);
            if (!is_dmadata(e0, e1, e2, candidate, rom_size)) {
                continue;
            }

            // Read in the DMADATA info.
            uint32_t const span = e2.vrom_end - e2.vrom_start;
            *info = (struct zelda64_dmadata_info){
                .offset = candidate,
                .size = span,
                .count = span / ZELDA64_DMA_ENTRY_SIZE
            };

            return ZELDA64_OK;
        }

        // Not in this chunk, continue to the next.
        offset += want;
    }

    return zelda64_set_error(error, ZELDA64_NO_DMADATA);
}

enum zelda64_result
zelda64_read_dmadata(struct zelda64_rom* rom, struct zelda64_error* error) {
    assert(rom != NULL);
    assert(error != NULL);

    // Attempt to find the DMADATA. If we can't find it, this ROM is definitely
    // not DMADATA so we should just quit instead.
    struct zelda64_dmadata_info info = {0};
    if (zelda64_find_dmadata(&info, &rom->source, error) != ZELDA64_OK) {
        return error->result;
    }

    // Allocate memory for our entries.
    struct zelda64_dmadata* entries = zelda64_alloc(rom->allocator, info.count * sizeof *entries);
    if (entries == NULL) {
        return zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
    }

    // And load in the data.
    for (size_t i = 0; i < info.count; i += READ_COUNT) {
        size_t const remaining = info.count - i;
        size_t const want = remaining < READ_COUNT ? remaining : READ_COUNT;

        // Get the entries from the ROM.
        zelda64_offset_t const offset = info.offset + i * ZELDA64_DMA_ENTRY_SIZE;
        uint8_t chunk[ZELDA64_DMA_ENTRY_SIZE * READ_COUNT];
        if (zelda64_source_read_exact(&rom->source, chunk, want * ZELDA64_DMA_ENTRY_SIZE, offset, error) < 0) {
            zelda64_free(rom->allocator, entries);
            return error->result;
        }

        for (size_t j = 0; j < want; ++j) {
            entries[i + j] = read_entry_unsafe(&chunk[j * ZELDA64_DMA_ENTRY_SIZE]);
        }
    }

    rom->dmadata_info = info;
    rom->dmadata = entries;
    return ZELDA64_OK;
}

size_t
zelda64_file_count(struct zelda64_rom const* rom) {
    assert(rom != NULL);
    return rom->dmadata_info.count;
}

struct zelda64_dmadata const*
zelda64_dmadata(struct zelda64_rom const* rom) {
    return rom->dmadata;
}

struct zelda64_dmadata const*
zelda64_dmadata_entry(struct zelda64_rom const* rom, zelda64_index_t const index,
                      struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    if (index >= rom->dmadata_info.count) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return NULL;
    }

    return &rom->dmadata[index];
}
