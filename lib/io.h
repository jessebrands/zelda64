/*
 * io.h: I/O abstraction
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

#ifndef LIBZELDA64_IO_H
#define LIBZELDA64_IO_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "zelda64/zelda64.h"

#include "allocator.h"
#include "error.h"

#define ZELDA64_SSIZE_MAX INT64_MAX

typedef size_t
(zelda64_read_func)(void* opaque,
                    void* buffer, size_t size,
                    zelda64_offset_t offset,
                    struct zelda64_error* error);

typedef size_t
(zelda64_write_func)(void* opaque,
                     void const* buffer, size_t size,
                     zelda64_offset_t offset,
                     struct zelda64_error* error);

typedef size_t
(zelda64_size_func)(void* opaque, struct zelda64_error* error);

typedef void
(zelda64_close_func)(void* opaque,
                     struct zelda64_allocator allocator,
                     struct zelda64_error* error);

struct zelda64_io {
    zelda64_read_func* read;
    zelda64_write_func* write;
    zelda64_size_func* size;
    zelda64_close_func* close;
    void* opaque;
    struct zelda64_allocator allocator;
};

static inline size_t
zelda64_io_read(struct zelda64_io const* io,
                void* buffer, size_t const size,
                zelda64_offset_t const offset,
                struct zelda64_error* error) {
    if (io == NULL || io->read == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    // An empty buffer is okay, so long as we're reading nothing.
    if (buffer == NULL && size > 0) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    return io->read(io->opaque, buffer, size, offset, error);
}

static inline size_t
zelda64_io_write(struct zelda64_io* io,
                 void const* buffer, size_t const size,
                 zelda64_offset_t const offset,
                 struct zelda64_error* error) {
    if (io == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }
    if (io->write == NULL) {
        zelda64_set_error(error, ZELDA64_UNSUPPORTED);
        return 0;
    }

    if (buffer == NULL && size > 0) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    return io->write(io->opaque, buffer, size, offset, error);
}

static inline size_t
zelda64_io_read_exact(struct zelda64_io const* io,
                      void* buffer, size_t const size,
                      zelda64_offset_t const offset,
                      struct zelda64_error* error) {
    size_t const bytes_in = zelda64_io_read(io, buffer, size, offset, error);
    if (error->result != ZELDA64_OK) {
        return 0;
    }

    if (bytes_in != size) {
        zelda64_set_error(error, ZELDA64_TRUNCATED);
        return 0;
    }

    return bytes_in;
}

static inline size_t
zelda64_io_size(struct zelda64_io const* io, struct zelda64_error* error) {
    if (io == NULL || io->size == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    return io->size(io->opaque, error);
}

static inline void
zelda64_io_close(struct zelda64_io* io, struct zelda64_error* error) {
    if (io == NULL || io->close == NULL) {
        return;
    }

    io->close(io->opaque, io->allocator, error);
    io->opaque = NULL;
}

void
zelda64_io_fopen(struct zelda64_io* io,
                 char const* filename,
                 struct zelda64_allocator allocator,
                 struct zelda64_error* error);

void
zelda64_io_fopen_ro(struct zelda64_io* io,
                    char const* filename,
                    struct zelda64_allocator allocator,
                    struct zelda64_error* error);

void
zelda64_io_from_file_ro(struct zelda64_io* io,
                        FILE* handle,
                        struct zelda64_allocator allocator,
                        struct zelda64_error* error);

void
zelda64_io_from_buffer(struct zelda64_io* io,
                       uint8_t* data, size_t size,
                       struct zelda64_allocator allocator,
                       struct zelda64_error* error);

void
zelda64_io_from_const_buffer(struct zelda64_io* io,
                             uint8_t const* data, size_t size,
                             struct zelda64_allocator allocator,
                             struct zelda64_error* error);

#endif //LIBZELDA64_IO_H
