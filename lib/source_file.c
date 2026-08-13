/*
 * source_file.c: C standard library file source
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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#include "source.h"
#include "source_file.h"

struct zelda64_source_file {
    FILE* handle;
};

static zelda64_ssize_t
zelda64_source_file_read(void* opaque,
                         void* buffer, size_t size,
                         zelda64_offset_t offset,
                         struct zelda64_error* error);

static zelda64_ssize_t
zelda64_source_file_size(void* opaque, struct zelda64_error* error);

static void
zelda64_source_file_close(void* opaque, struct zelda64_allocator allocator);

enum zelda64_result
zelda64_source_file_open(struct zelda64_source* source,
                         char const* filename,
                         struct zelda64_allocator allocator,
                         struct zelda64_error* error) {
    if (source == NULL || filename == NULL) {
        return zelda64_return_error(error, ZELDA64_INVALID_PARAMETER);
    }

    // Allocate the file state struct.
    struct zelda64_source_file* file = zelda64_alloc(allocator, sizeof *file);
    if (file == NULL) {
        return zelda64_return_error(error, ZELDA64_MEMORY_ERROR);
    }

    // Open the file for binary reading.
    file->handle = fopen(filename, "rb");
    if (file->handle == NULL) {
        int const sys_error = errno;
        zelda64_free(allocator, file);
        return zelda64_return_sys_error(error, ZELDA64_ERRNO, sys_error);
    }

    // Set the source vtable and opaque.
    *source = (struct zelda64_source){
        .read = zelda64_source_file_read,
        .size = zelda64_source_file_size,
        .close = zelda64_source_file_close,
        .opaque = file
    };
    return ZELDA64_OK;
}

static zelda64_ssize_t
zelda64_source_file_read(void* opaque,
                         void* buffer, size_t const size,
                         zelda64_offset_t const offset,
                         struct zelda64_error* error) {
    struct zelda64_source_file const* file = opaque;

    // Start from a clean slate.
    clearerr(file->handle);
    errno = 0;

    // Seek to the offset given, however...
    // Standard C is pretty restricted when it comes to these things!
    if (offset > LONG_MAX) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return -1;
    }
    if (fseek(file->handle, (long) offset, SEEK_SET) != 0) {
        zelda64_set_errno(error);
        return -1;
    }

    // Perform the read and check for errors.
    errno = 0;
    size_t const in_count = fread(buffer, sizeof(uint8_t), size, file->handle);
    int const sys_error = errno; // capture errno cause ferror can set it
    if (ferror(file->handle)) {
        zelda64_set_sys_error(error, ZELDA64_ERRNO, sys_error);
        return -1;
    }

    return (zelda64_ssize_t) in_count;
}

static zelda64_ssize_t
zelda64_source_file_size(void* opaque, struct zelda64_error* error) {
    struct zelda64_source_file const* file = opaque;

    // Start from a clean slate.
    clearerr(file->handle);
    errno = 0;

    // Seek to the end of the file.
    if (fseek(file->handle, 0, SEEK_END) != 0) {
        zelda64_set_errno(error);
        return -1;
    }

    // The position here is equal to the file size.
    errno = 0;
    long const position = ftell(file->handle);
    if (position < 0) {
        zelda64_set_errno(error);
        return -1;
    }

    return position;
}

static void
zelda64_source_file_close(void* opaque, struct zelda64_allocator allocator) {
    struct zelda64_source_file* file = opaque;
    fclose(file->handle);
    zelda64_free(allocator, file);
}
