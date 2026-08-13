/*
 * source_view.h: non-owning views over a source
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

#ifndef LIBZELDA64_SOURCE_VIEW_H
#define LIBZELDA64_SOURCE_VIEW_H

#include "source.h"

int
zelda64_source_view_from(struct zelda64_source* view,
                         zelda64_offset_t offset, size_t size,
                         struct zelda64_source const* source,
                         struct zelda64_allocator allocator,
                         struct zelda64_error* error);


#endif //LIBZELDA64_SOURCE_VIEW_H
