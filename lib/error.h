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

#include "zelda64/zelda64.h"

static inline void
zelda64_set_error(struct zelda64_error* error, enum zelda64_result const result) {
    if (error != NULL) {
        error->result = result;
        error->sys_error = 0;
    }
}

#endif //LIBZELDA64_ERROR_H
