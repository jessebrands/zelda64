/*
 * rom.h: Nintendo 64 Zelda ROM abstraction
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

#ifndef LIBZELDA64_ROM_H
#define LIBZELDA64_ROM_H

#include "zelda64/zelda64.h"

#include "source.h"

#define ZELDA64_DMA_ENTRY_SIZE 16

enum zelda64_dmadata_kind {
    ZELDA64_ENTRY_DELETED,
    ZELDA64_ENTRY_STORED,
    ZELDA64_ENTRY_COMPRESSED,
};

struct zelda64_dmadata_info {
    uint32_t offset;
    uint32_t size;
    size_t count;
};

struct zelda64_rom {
    struct zelda64_allocator allocator;
    struct zelda64_source source;

    struct zelda64_dmadata_info dmadata_info;
    struct zelda64_dmadata* dmadata;
    enum zelda64_cic cic;
};

enum zelda64_dmadata_kind
zelda64_dmadata_kind(struct zelda64_dmadata const* entry);

enum zelda64_result
zelda64_read_dmadata(struct zelda64_rom* rom,
                     struct zelda64_error* error);

enum zelda64_cic
zelda64_detect_cic(struct zelda64_rom const* rom,
                   struct zelda64_error* error);

uint64_t
zelda64_calculate_check_code(struct zelda64_source const* source,
                             struct zelda64_dmadata const* dmadata,
                             size_t dmadata_count,
                             struct zelda64_error* error);

#endif //LIBZELDA64_ROM_H
