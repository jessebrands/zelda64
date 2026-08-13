/*
 * rom_open.c: ROM opening routines
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

#include "zelda64/zelda64.h"

#include "allocator.h"
#include "error.h"
#include "rom.h"
#include "source_file.h"

struct zelda64_rom*
zelda64_open(char const* filename, struct zelda64_error* error) {
    struct zelda64_allocator const allocator = zelda64_default_allocator();
    return zelda64_open_with_allocator(filename, allocator, error);
}

struct zelda64_rom*
zelda64_open_with_allocator(char const* filename,
                            struct zelda64_allocator const allocator,
                            struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    // Can't open a file we don't have.
    if (filename == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return NULL;
    }

    // Callers must set an allocator if they call this function.
    if ((allocator.alloc == NULL) || (allocator.free == NULL)) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return NULL;
    }

    // Allocate memory for the ROM state.
    struct zelda64_rom* rom = zelda64_alloc(allocator, sizeof *rom);
    if (rom == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        return NULL;
    }

    *rom = (struct zelda64_rom){0};
    rom->allocator = allocator;

    // Open the ROM source file.
    if (zelda64_source_file_open(&rom->source, filename, allocator, error) < 0) {
        zelda64_close(rom);
        return NULL;
    }

    if (zelda64_read_dmadata(rom, error) != ZELDA64_OK) {
        zelda64_close(rom);
        return NULL;
    }

    return rom;
}

void
zelda64_close(struct zelda64_rom* rom) {
    if (rom == NULL) {
        return;
    }

    zelda64_free(rom->allocator, rom->dmadata);
    zelda64_source_close(&rom->source, rom->allocator);
    zelda64_free(rom->allocator, rom);
}
