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
#include "io.h"

static struct zelda64_rom*
open_from_io(struct zelda64_io io,
             struct zelda64_allocator const allocator,
             struct zelda64_error* error) {
    // Allocate memory for the ROM state.
    struct zelda64_rom* rom = zelda64_alloc(allocator, sizeof *rom);
    if (rom == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        zelda64_io_close(&io);
        return NULL;
    }

    *rom = (struct zelda64_rom){
        .allocator = allocator,
        .io = io,
    };

    // If we can't find the DMADATA in this ROM, then it's not a Zelda 64 ROM.
    if (zelda64_read_dmadata(rom, error) != ZELDA64_OK) {
        zelda64_close(rom);
        return NULL;
    }

    // Detect the CIC chip for this ROM.
    rom->cic = zelda64_detect_cic(rom, error);
    if (rom->cic == ZELDA64_CIC_UNKNOWN) {
        zelda64_close(rom);
        return NULL;
    }

    return rom;
}

struct zelda64_rom*
zelda64_open(char const* filename, struct zelda64_error* error) {
    struct zelda64_allocator const allocator = zelda64_default_allocator();
    return zelda64_open_with_allocator(filename, allocator, error);
}

struct zelda64_rom*
zelda64_open_with_allocator(char const* filename,
                            struct zelda64_allocator const allocator,
                            struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }

    // Can't open a file we don't have.
    if (filename == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return NULL;
    }

    // Open the ROM file.
    struct zelda64_io file = {0};
    zelda64_io_fopen_ro(&file, filename, allocator, error);
    if (ZELDA64_FAILED(error)) {
        return NULL;
    }

    // Open the ROM from the I/O object.
    struct zelda64_rom* rom = open_from_io(file, allocator, error);
    if (ZELDA64_FAILED(error)) {
        return NULL;
    }

    zelda64_clear_error(error);
    return rom;
}

struct zelda64_rom*
zelda64_open_buffer(uint8_t const* data, size_t size,
                    struct zelda64_error* error) {
    struct zelda64_allocator const allocator = zelda64_default_allocator();
    return zelda64_open_buffer_with_allocator(data, size, allocator, error);
}

struct zelda64_rom*
zelda64_open_buffer_with_allocator(uint8_t const* data, size_t const size,
                                   struct zelda64_allocator const allocator,
                                   struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }

    // Wrap the buffer in an I/O object.
    struct zelda64_io file = {0};
    zelda64_io_from_const_buffer(&file, data, size, allocator, error);
    if (ZELDA64_FAILED(error)) {
        return NULL;
    }

    // Create the ROM from the I/O object.
    struct zelda64_rom* rom = open_from_io(file, allocator, error);
    if (ZELDA64_FAILED(error)) {
        return NULL;
    }

    zelda64_clear_error(error);
    return rom;
}

void
zelda64_close(struct zelda64_rom* rom) {
    if (rom == NULL) {
        return;
    }

    zelda64_free(rom->allocator, rom->dmadata);
    zelda64_io_close(&rom->io);
    zelda64_free(rom->allocator, rom);
}

size_t
zelda64_rom_size(struct zelda64_rom const* rom, struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    if (rom == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    size_t const size = zelda64_io_size(&rom->io, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    zelda64_clear_error(error);
    return size;
}
