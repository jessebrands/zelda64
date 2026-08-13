/*
 * rom_decompress.c: generate decompressed ROM layouts
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

#include "allocator.h"
#include "error.h"
#include "layout.h"
#include "zelda64/zelda64.h"

struct zelda64_dmadata_layout*
zelda64_decompress(struct zelda64_rom const* rom, struct zelda64_error* error) {
    struct zelda64_allocator const allocator = zelda64_default_allocator();
    return zelda64_decompress_with_allocator(rom, allocator, error);
}

struct zelda64_dmadata_layout*
zelda64_decompress_with_allocator(struct zelda64_rom const* rom,
                                      struct zelda64_allocator const allocator,
                                      struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    // Can't generate DMADATA layout from nothing.
    if (rom == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return NULL;
    }

    struct zelda64_dmadata_layout* layout = zelda64_alloc(allocator, sizeof *layout);
    if (layout == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        return NULL;
    }

    *layout = (struct zelda64_dmadata_layout){
        .allocator = allocator,
    };
    return layout;
}
