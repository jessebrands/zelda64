/*
 * layout_write.c: write a Nintendo 64 Zelda ROM from layout
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

#include <stdio.h>

#include "allocator.h"
#include "error.h"
#include "layout.h"
#include "rom.h"
#include "zelda64/zelda64.h"

enum zelda64_result
zelda64_write(char const* filename,
              struct zelda64_dmadata_layout const* layout,
              struct zelda64_write_options const* options,
              struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    if (filename == NULL || layout == NULL || options == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }

    // Get the size of the DMADATA we're making.
    size_t const count = layout->count;

    // We'll need to allocate a DMADATA for ourselves.
    struct zelda64_dmadata* dmadata = zelda64_alloc(layout->allocator, count * sizeof *dmadata);
    if (dmadata == NULL) {
        return zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
    }

    // Create our new ROM.
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        zelda64_set_errno(error);
        zelda64_free(layout->allocator, dmadata);
        return ZELDA64_ERRNO;
    }

    // Enter the writing loop, this is where things get complex.
    uint32_t position = 0;
    for (size_t i = 0; i < count; ++i) {
        struct zelda64_layout_entry const* entry = &layout->entries[i];

        // If this entry has been deleted, we can take this fast path.
        if (entry->operation == ZELDA64_OP_DELETE) {
            dmadata[i] = (struct zelda64_dmadata){
                .vrom_start = entry->vrom_start,
                .vrom_end = entry->vrom_end,
                .rom_start = UINT32_MAX,
                .rom_end = UINT32_MAX,
            };
            continue;
        }

        // Empty files are a special case.
        if (entry->vrom_start == entry->vrom_end) {
            dmadata[i] = (struct zelda64_dmadata){0};
            continue;
        }

        // If we're packing sparsely, we'll write to where the VROM says.
        uint32_t const rom_start = (options->pack == ZELDA64_PACK_SPARSE)
                                       ? entry->vrom_start
                                       : position;

        // TODO: Handle the actual file here.

        dmadata[i] = (struct zelda64_dmadata){
            .vrom_start = entry->vrom_start,
            .vrom_end = entry->vrom_end,
            .rom_start = rom_start,
            .rom_end = 0,
        };

        position = rom_start;
    }

    // Now we must write the DMADATA to the ROM.
    // The DMADATA is at entry 0x0002, so we need to fit exactly there.
    struct zelda64_dmadata const* e_dmadata = &dmadata[0x0002];
    fseek(file, (long) e_dmadata->rom_start, SEEK_SET);

    // TODO: Need DMADATA writing function. :o

    // TODO: Calculate the ROM check code.

    // We're done, clean up!
    zelda64_free(layout->allocator, dmadata);
    fclose(file);
    return ZELDA64_OK;
}
