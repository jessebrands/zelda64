/*
 * manifest.c: manifest file operations
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of zelda64.
 *
 * zelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * zelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LIBZELDA64_MANIFEST_H
#define LIBZELDA64_MANIFEST_H

#include <zelda64/zmf.h>

#include "rom.h"

enum zelda64_operation {
    ZELDA64_OPERATION_PASS,
    ZELDA64_OPERATION_COPY,
    ZELDA64_OPERATION_COMPRESS,
};

struct zelda64_manifest {
    char const* filename;
    FILE* file;
    size_t file_size;
    struct zelda64_zmf_header header;

    uint8_t* copy_list;
    size_t copy_list_size;
};

enum zelda64_result
zelda64_open_manifest(char const* filename, struct zelda64_manifest* manifest);

void
zelda64_close_manifest(struct zelda64_manifest* manifest);

enum zelda64_result
zelda64_make_rom_manifest(char const* filename, struct zelda64_rom const* rom);

enum zelda64_result
zelda64_manifest_check_input_rom(struct zelda64_manifest const* manifest,
                           struct zelda64_rom const* rom);

enum zelda64_result
zelda64_manifest_check_output_rom(struct zelda64_manifest const* manifest,
                                  struct zelda64_rom const* rom);

enum zelda64_operation
zelda64_manifest_operation(struct zelda64_manifest const* manifest,
                           struct zelda64_dma_entry const* entry, size_t index);

#endif //LIBZELDA64_MANIFEST_H
