/*
 * rom.h: ROM file operations
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

#ifndef ZELDA64_ROM_FILE_H
#define ZELDA64_ROM_FILE_H

#include <stdio.h>

#include <zelda64/zelda64.h>

#define COMPRESSED_SIZE   0x2000000u
#define DECOMPRESSED_SIZE 0x4000000u

struct zelda64_rom {
    char const* filename;
    FILE* file;
    size_t file_size;
    struct zelda64_dmadata_info dma_info;
    struct zelda64_rom_info info;
    struct zelda64_dma_entry* dma_table;
};

enum zelda64_result
zelda64_open_rom(char const* filename, struct zelda64_rom* rom);

enum zelda64_result
zelda64_create_rom(char const* filename, size_t file_size,
                   struct zelda64_dmadata_info const* dma_info,
                   struct zelda64_rom* rom);

void
zelda64_close_rom(struct zelda64_rom* rom);

enum zelda64_result
zelda64_copy_file(struct zelda64_rom* out_rom, uint32_t offset,
                  struct zelda64_rom const* in_rom,
                  size_t file_index);

enum zelda64_result
zelda64_compress_file(struct zelda64_rom* out_rom, uint32_t offset,
                      struct zelda64_rom const* in_rom,
                      size_t file_index);

enum zelda64_result
zelda64_decompress_file(struct zelda64_rom* out_rom,
                        struct zelda64_rom const* in_rom,
                        size_t file_index);

enum zelda64_result
zelda64_write_dmadata_to_rom(struct zelda64_rom* rom);

enum zelda64_result
zelda64_finalize_rom(struct zelda64_rom* rom);

#endif //ZELDA64_ROM_FILE_H
