/*
 * allocator.c: C standard library allocator
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

#include <stdlib.h>

#include "zelda64/zelda64.h"

static void*
zelda64_default_alloc(void* opaque, size_t const size) {
    (void) opaque;
    return malloc(size);
}

static void
zelda64_default_free(void* opaque, void* ptr) {
    (void) opaque;
    free(ptr);
}

struct zelda64_allocator zelda64_default_allocator(void) {
    return (struct zelda64_allocator) {
        .alloc = zelda64_default_alloc,
        .free = zelda64_default_free,
        .opaque = NULL,
    };
}
