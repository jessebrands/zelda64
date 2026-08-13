/*
 * source_view.c: non-owning views over a source
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

#include <assert.h>

#include "source_view.h"

struct zelda64_source_view {
    struct zelda64_source const* source;
    zelda64_offset_t offset;
    size_t size;
};

static zelda64_ssize_t
zelda64_source_view_read(void* opaque,
                         void* buffer, size_t size,
                         zelda64_offset_t offset,
                         struct zelda64_error* error);

static zelda64_ssize_t
zelda64_source_view_size(void* opaque, struct zelda64_error* error);

static void
zelda64_source_view_close(void* opaque, struct zelda64_allocator allocator);

int
zelda64_source_view_from(struct zelda64_source* view,
                         zelda64_offset_t const offset, size_t const size,
                         struct zelda64_source const* source,
                         struct zelda64_allocator const allocator,
                         struct zelda64_error* error) {
    assert(view != NULL);
    assert(error != NULL);
    assert(source != NULL);

    // Ensure that the view is even expressible at all.
    if (size > SIZE_MAX - offset || size > ZELDA64_SSIZE_MAX) {
        return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
    }

    // Allocate space for the state itself.
    struct zelda64_source_view* state = zelda64_alloc(allocator, sizeof *state);
    if (state == NULL) {
        return zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
    }

    *state = (struct zelda64_source_view){
        .source = source,
        .offset = offset,
        .size = size,
    };
    *view = (struct zelda64_source){
        .read = zelda64_source_view_read,
        .size = zelda64_source_view_size,
        .close = zelda64_source_view_close,
        .opaque = state,
    };

    return ZELDA64_OK;
}

static zelda64_ssize_t
zelda64_source_view_read(void* opaque,
                         void* buffer, size_t const size,
                         zelda64_offset_t const offset,
                         struct zelda64_error* error) {
    struct zelda64_source_view const* view = opaque;

    if (offset >= view->size) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return 0;
    }

    size_t const available = view->size - offset;
    size_t const want = size < available ? size : available;

    return zelda64_source_read(view->source, buffer, want, view->offset + offset, error);
}

static zelda64_ssize_t
zelda64_source_view_size(void* opaque, struct zelda64_error* error) {
    (void) error;
    struct zelda64_source_view const* view = opaque;
    return (zelda64_ssize_t) view->size;
}

static void
zelda64_source_view_close(void* opaque, struct zelda64_allocator allocator) {
    zelda64_free(allocator, opaque);
}
