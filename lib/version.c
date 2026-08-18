/*
 * version.c: version query functions
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

#include <stdio.h>
#include <yaz0/yaz0.h>

#include "zelda64/zelda64.h"

int
zelda64_version(void) {
    return ZELDA64_VERSION;
}

char const*
zelda64_version_string(void) {
    return ZELDA64_VERSION_STRING;
}

void zelda64_compressor_name(char* dst, size_t const size) {
    snprintf(dst, size, "libyaz0 %s (%s)",
        yaz0_version_string(),
        yaz0_search_name(yaz0_default_search()));
}
