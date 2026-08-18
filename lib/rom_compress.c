/*
 * rom_compress.c: generate a compressed layout from a ROM
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

#include "layout.h"
#include "rom.h"
#include "zelda64/zelda64.h"

static struct zelda64_layout_entry*
compress_rom(struct zelda64_rom const* rom,
             struct zelda64_allocator const allocator,
             struct zelda64_error* error) {
    size_t const count = rom->dmadata_info.count;
    struct zelda64_layout_entry* entries = zelda64_alloc(
        allocator,
        count * sizeof *entries
    );

    if (entries == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        return NULL;
    }

    for (size_t i = 0; i < count; ++i) {
        struct zelda64_dmadata const* dma = &rom->dmadata[i];
        struct zelda64_layout_entry* entry = &entries[i];

        entry->vrom_start = dma->vrom_start;
        entry->vrom_end = dma->vrom_end;


        enum zelda64_dmadata_kind const kind = zelda64_dmadata_kind(dma);
        switch (kind) {
            case ZELDA64_ENTRY_DELETED:
                entry->operation = ZELDA64_OP_DELETE;
                break;

            case ZELDA64_ENTRY_STORED:
                entry->operation = ZELDA64_OP_COMPRESS;
                break;

            case ZELDA64_ENTRY_COMPRESSED:
                entry->operation = ZELDA64_OP_COPY;
                break;
        }

        // First 3 files are always copied, never compressed.
        if (i < 3) {
            entries[i].operation = ZELDA64_OP_COPY;
        }

        entry->from_type = ZELDA64_FROM_ROM;
        entry->from.rom.rom = rom;
        entry->from.rom.index = i;
    }

    return entries;
}

struct zelda64_dmadata_layout*
zelda64_compress(struct zelda64_rom const* rom, struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }
    if (rom == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return NULL;
    }

    return zelda64_compress_with_allocator(rom, rom->allocator, error);
}

struct zelda64_dmadata_layout*
zelda64_compress_with_allocator(struct zelda64_rom const* rom,
                                  struct zelda64_allocator const allocator,
                                  struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    // Can't generate a layout from nothing.
    if (rom == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return NULL;
    }

    struct zelda64_layout_entry* entries = compress_rom(rom, allocator, error);
    if (entries == NULL) {
        return NULL;
    }

    // Allocate memory for our layout.
    struct zelda64_dmadata_layout* layout = zelda64_alloc(allocator, sizeof *layout);
    if (layout == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        zelda64_free(allocator, entries);
        return NULL;
    }

    *layout = (struct zelda64_dmadata_layout){
        .allocator = allocator,
        .count = rom->dmadata_info.count,
        .entries = entries,
    };

    zelda64_clear_error(error);
    return layout;
}
