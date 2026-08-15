/*
 * bytes.h: byte manipulation functions
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

#ifndef LIBZELDA64_BYTES_H
#define LIBZELDA64_BYTES_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static inline uint32_t
zelda64_read_u32(uint8_t const* p) {
    assert(p != NULL);

    return ((uint32_t) p[0] << 24)
         | ((uint32_t) p[1] << 16)
         | ((uint32_t) p[2] << 8)
         | ((uint32_t) p[3]);
}

static inline void
zelda64_write_u32(uint8_t* p, uint32_t const n) {
    assert(p != NULL);

    p[0] = (uint8_t) (n >> 24);
    p[1] = (uint8_t) (n >> 16);
    p[2] = (uint8_t) (n >> 8);
    p[3] = (uint8_t) (n);
}

static inline void
zelda64_write_u64(uint8_t* p, uint64_t const n) {
    assert(p != NULL);

    p[0] = (uint8_t) (n >> 56);
    p[1] = (uint8_t) (n >> 48);
    p[2] = (uint8_t) (n >> 40);
    p[3] = (uint8_t) (n >> 32);
    p[4] = (uint8_t) (n >> 24);
    p[5] = (uint8_t) (n >> 16);
    p[6] = (uint8_t) (n >> 8);
    p[7] = (uint8_t) (n);
}

static inline uint32_t
zelda64_rot32(uint32_t const a, unsigned const b) {
    return a << b | a >> ((32 - b) & 31);
}

#endif //LIBZELDA64_BYTES_H
