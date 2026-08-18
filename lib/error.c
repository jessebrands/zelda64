/*
 * error.h: error functions
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
#include <yaz0/yaz0.h>

#include "error.h"

static char const*
zelda64_result_string(enum zelda64_result const result) {
    switch (result) {
        case ZELDA64_OK: return "ok";
        case ZELDA64_INVALID_PARAMETER: return "invalid parameter";
        case ZELDA64_MEMORY_ERROR: return "out of memory";
        case ZELDA64_OUT_OF_RANGE: return "out of range";
        case ZELDA64_TRUNCATED: return "truncated";
        case ZELDA64_ERRNO: return "system error";
        case ZELDA64_NO_DMADATA: return "no DMADATA";
        case ZELDA64_DELETED: return "deleted";
        case ZELDA64_DECOMPRESS_ERROR: return "decompression error";
        case ZELDA64_COMPRESS_ERROR: return "compression error";
        case ZELDA64_INVALID_ROM: return "invalid ROM";
        case ZELDA64_UNSUPPORTED_CIC: return "unknown CIC";
        case ZELDA64_UNSUPPORTED: return "unsupported operation";
    }
    return "unknown";
}

char const*
zelda64_error_string(struct zelda64_error const* error) {
    if (error == NULL) {
        return "(null)";
    }

    if (error->result == ZELDA64_ERRNO) {
        return strerror(error->sys_error);
    }
    if (error->result == ZELDA64_DECOMPRESS_ERROR) {
        return yaz0_result_string(error->sys_error);
    }

    // No system error, so just get our own result.
    return zelda64_result_string(error->result);
}
