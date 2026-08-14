/*
 * rom_read.c: ROM file reading functions
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

#include "error.h"
#include "rom.h"
#include "zelda64/zelda64.h"

zelda64_ssize_t
zelda64_read_storage(void* buffer, size_t const size,
                     struct zelda64_rom const* rom,
                     zelda64_index_t const index, uint32_t const offset,
                     struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    // Can't read from nothing, nor can we write to nothing.
    if (rom == NULL || (buffer == NULL && size > 0)) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return -1;
    }

    // Get information about this file.
    struct zelda64_stat st;
    if (zelda64_stat(&st, rom, index, error) != ZELDA64_OK) {
        return -1;
    }

    // Reading past the end of the file is not an error,
    // we just don't read any bytes at all.
    if (offset >= st.size) {
        return 0;
    }

    // Set up the read.
    size_t const available = st.size - offset;
    size_t const want = available < size ? available : size;
    return zelda64_source_read(
        &rom->source,
        buffer, want,
        (zelda64_offset_t) st.offset + offset,
        error
    );
}
