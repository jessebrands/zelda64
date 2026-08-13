/*
 * source.h: data source abstraction
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

#ifndef LIBZELDA64_SOURCE_H
#define LIBZELDA64_SOURCE_H

#include <stddef.h>
#include <stdint.h>

#include "zelda64/zelda64.h"

#include "allocator.h"
#include "error.h"

#define ZELDA64_SSIZE_MAX INT64_MAX

typedef size_t zelda64_offset_t;
typedef int64_t zelda64_ssize_t;

typedef zelda64_ssize_t
(zelda64_read_func)(void* opaque,
                    void* buffer, size_t size,
                    zelda64_offset_t offset,
                    struct zelda64_error* error);

typedef zelda64_ssize_t
(zelda64_size_func)(void* opaque, struct zelda64_error* error);

typedef void
(zelda64_close_func)(void* opaque, struct zelda64_allocator allocator);

struct zelda64_source {
    zelda64_read_func* read;
    zelda64_size_func* size;
    zelda64_close_func* close;
    void* opaque;
};

static inline zelda64_ssize_t
zelda64_source_read(struct zelda64_source const* source,
                    void* buffer, size_t const size,
                    zelda64_offset_t const offset,
                    struct zelda64_error* error) {
    if (source == NULL || source->read == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return -1;
    }

    // Can we return a read count at all?
    if (size > ZELDA64_SSIZE_MAX) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return -1;
    }

    // An empty buffer is okay, so long as we're reading nothing.
    if (buffer == NULL && size > 0) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return -1;
    }

    return source->read(source->opaque, buffer, size, offset, error);
}

static inline zelda64_ssize_t
zelda64_source_read_exact(struct zelda64_source const* source,
                          void* buffer, size_t const size,
                          zelda64_offset_t const offset,
                          struct zelda64_error* error) {
    zelda64_ssize_t const bytes_in = zelda64_source_read(source, buffer, size, offset, error);
    if (bytes_in < 0) {
        return bytes_in;
    }

    if (bytes_in != (zelda64_ssize_t) size) {
        zelda64_set_error(error, ZELDA64_TRUNCATED);
        return -1;
    }

    return bytes_in;
}

static inline zelda64_ssize_t
zelda64_source_size(struct zelda64_source const* source, struct zelda64_error* error) {
    if (source == NULL || source->size == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return -1;
    }

    return source->size(source->opaque, error);
}

static inline void
zelda64_source_close(struct zelda64_source* source, struct zelda64_allocator const allocator) {
    if (source == NULL || source->close == NULL) {
        return;
    }

    source->close(source->opaque, allocator);
    source->opaque = NULL;
}

#endif //LIBZELDA64_SOURCE_H
