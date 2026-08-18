/*
 * layout.h: DMADATA layout structure
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

#ifndef LIBZELDA64_LAYOUT_H
#define LIBZELDA64_LAYOUT_H

#include "rom.h"
#include "zelda64/zelda64.h"

enum zelda64_from_type {
    ZELDA64_FROM_ROM = 0,
};

struct zelda64_layout_entry {
    uint32_t vrom_start;
    uint32_t vrom_end;

    enum zelda64_operation operation;
    enum zelda64_from_type from_type;

    union {
        struct {
            struct zelda64_rom const* rom;
            zelda64_index_t index;
        } rom;
    } from;
};

struct zelda64_dmadata_layout {
    struct zelda64_allocator allocator;
    size_t count;
    struct zelda64_layout_entry* entries;
};

uint32_t
zelda64_layout_entry_bound(struct zelda64_layout_entry const* entry,
                           struct zelda64_error* error);

#endif //LIBZELDA64_LAYOUT_H
