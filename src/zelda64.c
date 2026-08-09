/*
 * zelda64.c: Nintendo 64 Zelda ROM manipulation common functions
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

#include "zelda64/zelda64.h"

char const* zelda64_result_string(enum zelda64_result const result) {
    switch (result) {
        case ZELDA64_OK: return "ok";
        case ZELDA64_INVALID_PARAMETER: return "invalid parameter";
        case ZELDA64_OUT_OF_RANGE: return "out of range";
        case ZELDA64_NO_DMADATA: return "ROM has no DMADATA";
        case ZELDA64_MEMORY_ERROR: return "out of memory";
        case ZELDA64_COMPRESSION_ERROR: return "compression error";
        case ZELDA64_DECOMPRESSION_ERROR: return "decompression error";
        case ZELDA64_IO_ERROR: return "i/o error";
        case ZELDA64_BAD_MANIFEST: return "bad manifest file";
        case ZELDA64_UNSUPPORTED_VERSION: return "unsupported version";
        case ZELDA64_NOT_FOUND: return "not found";
    }
    return "unknown";
}

char const* zelda64_version_string(void) {
    return ZELDA64_VERSION_STRING;
}
