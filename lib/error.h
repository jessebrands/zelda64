/*
 * error.h: error setting utilities
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

#ifndef LIBZELDA64_ERROR_H
#define LIBZELDA64_ERROR_H

#include <assert.h>
#include <errno.h>

#include "zelda64/zelda64.h"

#define ZELDA64_SUCCESS(x) (x->result == ZELDA64_OK)
#define ZELDA64_FAILED(x) (x->result != ZELDA64_OK)

static inline enum zelda64_result
zelda64_set_error(struct zelda64_error* error, enum zelda64_result const result) {
    assert(error != NULL);
    assert(result != ZELDA64_OK);

    error->result = result;
    error->sys_error = 0;
    return result;
}

static inline enum zelda64_result
zelda64_set_sys_error(struct zelda64_error* error,
                      enum zelda64_result const result, int const sys_error) {
    assert(error != NULL);
    assert(result != ZELDA64_OK);
    assert(sys_error != 0);

    error->result = result;
    error->sys_error = sys_error;
    return result;
}

static inline enum zelda64_result
zelda64_set_errno(struct zelda64_error* error) {
    assert(error != NULL);

    error->result = ZELDA64_ERRNO;
    error->sys_error = errno;
    return ZELDA64_ERRNO;
}

#endif //LIBZELDA64_ERROR_H
