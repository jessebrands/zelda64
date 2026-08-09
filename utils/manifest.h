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

#include "rom.h"

enum zelda64_result
zelda64_make_rom_manifest(char const* filename, struct zelda64_rom const* rom);

enum zelda64_result
zelda64_read_rom_op_list(char const* filename, struct zelda64_rom const* rom,
                         uint8_t* ops, size_t count);

#endif //LIBZELDA64_MANIFEST_H
