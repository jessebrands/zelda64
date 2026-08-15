/*
 * layout_operation.c: layout operation modifiers
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

#include "layout.h"
#include "zelda64/zelda64.h"


void
zelda64_layout_set_operation(struct zelda64_dmadata_layout* layout,
                             zelda64_index_t const index, enum zelda64_operation const operation,
                             struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    if (layout == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return;
    }

    if (index > layout->count) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return;
    }

    layout->entries[index].operation = operation;
}