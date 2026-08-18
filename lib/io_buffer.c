/*
 * io_buffer.c: buffer based I/O
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

#include <string.h>

#include "io.h"

struct zelda64_io_buffer {
    uint8_t* data;
    size_t size;
};

static size_t
zelda64_buffer_read(void* opaque,
                    void* dst, size_t size,
                    zelda64_offset_t offset,
                    struct zelda64_error* error);

static size_t
zelda64_buffer_write(void* opaque,
                     void const* src, size_t size,
                     zelda64_offset_t offset,
                     struct zelda64_error* error);

static size_t
zelda64_buffer_size(void* opaque, struct zelda64_error* error);

static void
zelda64_buffer_close(void* opaque,
                     struct zelda64_allocator allocator,
                     struct zelda64_error* error);

static struct zelda64_io_buffer*
init_io_buffer(struct zelda64_allocator const allocator,
               struct zelda64_error* error) {
    struct zelda64_io_buffer* buffer = zelda64_alloc(allocator, sizeof *buffer);
    if (buffer == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        return NULL;
    }
    return buffer;
}

void
zelda64_io_from_buffer(struct zelda64_io* io,
                       uint8_t* data, size_t const size,
                       struct zelda64_allocator const allocator,
                       struct zelda64_error* error) {
    if (io == NULL || (data == NULL && size > 0)) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return;
    }
    struct zelda64_io_buffer* buffer = init_io_buffer(allocator, error);
    if (buffer == NULL) {
        return;
    }

    buffer->data = data;
    buffer->size = size;
    *io = (struct zelda64_io){
        .read = zelda64_buffer_read,
        .write = zelda64_buffer_write,
        .size = zelda64_buffer_size,
        .close = zelda64_buffer_close,
        .opaque = buffer,
        .allocator = allocator,
    };
}

void
zelda64_io_from_const_buffer(struct zelda64_io* io,
                             uint8_t const* data, size_t const size,
                             struct zelda64_allocator const allocator,
                             struct zelda64_error* error) {
    if (io == NULL || (data == NULL && size > 0)) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return;
    }
    struct zelda64_io_buffer* buffer = init_io_buffer(allocator, error);
    if (buffer == NULL) {
        return;
    }
    buffer->data = (uint8_t*) data; // This is safe, cause we never write.
    buffer->size = size;
    *io = (struct zelda64_io){
        .read = zelda64_buffer_read,
        .write = NULL,
        .size = zelda64_buffer_size,
        .close = zelda64_buffer_close,
        .opaque = buffer,
        .allocator = allocator,
    };
}

static size_t
zelda64_buffer_read(void* opaque,
                    void* dst, size_t const size,
                    zelda64_offset_t const offset,
                    struct zelda64_error* error) {
    (void) error;
    struct zelda64_io_buffer* buffer = opaque;

    // Over-reads are not an error.
    if (offset >= buffer->size) {
        return 0;
    }

    // Read as many bytes as we can into the buffer.
    size_t const available = buffer->size - offset;
    size_t const want = size < available ? size : available;
    memcpy(dst, &buffer->data[offset], want);
    return want;
}

static size_t
zelda64_buffer_write(void* opaque,
                     void const* src, size_t size,
                     zelda64_offset_t offset,
                     struct zelda64_error* error) {
    struct zelda64_io_buffer* buffer = opaque;
    if (offset > buffer->size) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return 0;
    }
    if (size > buffer->size - offset) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return 0;
    }
    memcpy(&buffer->data[offset], src, size);
    return size;
}


static size_t
zelda64_buffer_size(void* opaque, struct zelda64_error* error) {
    (void) error;
    struct zelda64_io_buffer* buffer = opaque;
    return buffer->size;
}

static void
zelda64_buffer_close(void* opaque,
                     struct zelda64_allocator const allocator,
                     struct zelda64_error* error) {
    (void) error;
    struct zelda64_io_buffer* buffer = opaque;
    zelda64_free(allocator, buffer);
}
