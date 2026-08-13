/*
 * source_file.h: C standard library file source
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

#ifndef LIBZELDA64_SOURCE_FILE_H
#define LIBZELDA64_SOURCE_FILE_H

#include "zelda64/zelda64.h"

#include "source.h"

enum zelda64_result
zelda64_source_file_open(struct zelda64_source* source,
                         char const* filename,
                         struct zelda64_allocator allocator,
                         struct zelda64_error* error);

#endif //LIBZELDA64_SOURCE_FILE_H
