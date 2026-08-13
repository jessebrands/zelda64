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

struct zelda64_dma_entry {
    uint32_t vrom_start;
    uint32_t vrom_end;
    uint32_t rom_start;
    uint32_t rom_end;
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
    struct zelda64_dma_entry* dmadata;
};

enum zelda64_result
zelda64_read_dmadata(struct zelda64_rom* rom,
                     struct zelda64_error* error);

enum zelda64_result
zelda64_rom_file(struct zelda64_source* file, struct zelda64_rom const* rom, size_t index,
                 struct zelda64_error* error);

#endif //LIBZELDA64_ROM_H
