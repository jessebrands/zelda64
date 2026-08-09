/*
 * zmf.c: zelda64 manifest library
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

#include "zelda64/zelda64.h"
#include "zelda64/zmf.h"

#include "bytes.h"

#define ZMF_OFFSET_MAGIC        0x00
#define ZMF_OFFSET_VERSION      0x04
#define ZMF_OFFSET_RESERVED0    0x08
#define ZMF_OFFSET_GAME_CODE    0x10
#define ZMF_OFFSET_GAME_VERSION 0x14
#define ZMF_OFFSET_RESERVED1    0x15
#define ZMF_OFFSET_CHECK_CODE   0x18
#define ZMF_OFFSET_PROGRAM      0x20

static char const zmf_magic[4] = {'Z', '6', '4', 'M'};

enum zelda64_result
zelda64_zmf_read_header(struct zelda64_zmf_header* header,
                        uint8_t const* data, size_t const size) {
    if (header == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_ZMF_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    if (memcmp(&data[ZMF_OFFSET_MAGIC], zmf_magic, sizeof zmf_magic) != 0) {
        return ZELDA64_BAD_MANIFEST;
    }

    header->version = zelda64_read_u32(&data[ZMF_OFFSET_VERSION]);
    memcpy(header->reserved0, &data[ZMF_OFFSET_RESERVED0], sizeof header->reserved0);
    memcpy(header->game_code, &data[ZMF_OFFSET_GAME_CODE], sizeof header->game_code);
    header->game_version = data[ZMF_OFFSET_GAME_VERSION];
    memcpy(header->reserved1, &data[ZMF_OFFSET_RESERVED1], sizeof header->reserved1);
    uint32_t const cc_high = zelda64_read_u32(&data[ZMF_OFFSET_CHECK_CODE]);
    uint32_t const cc_low = zelda64_read_u32(&data[ZMF_OFFSET_CHECK_CODE + 0x04]);
    header->check_code = ((uint64_t) cc_high << 32) | ((uint64_t) cc_low);
    memcpy(header->program, &data[ZMF_OFFSET_PROGRAM], sizeof header->program);

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_zmf_write_header(uint8_t* data, size_t const size,
                         struct zelda64_zmf_header const* header) {
    if (header == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_ZMF_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    memcpy(&data[ZMF_OFFSET_MAGIC], zmf_magic, sizeof zmf_magic);
    zelda64_write_u32(&data[ZMF_OFFSET_VERSION], header->version);
    memcpy(&data[ZMF_OFFSET_RESERVED0], header->reserved0, sizeof header->reserved0);
    memcpy(&data[ZMF_OFFSET_GAME_CODE], header->game_code, sizeof header->game_code);
    data[ZMF_OFFSET_GAME_VERSION] = header->game_version;
    memcpy(&data[ZMF_OFFSET_RESERVED1], header->reserved1, sizeof header->reserved1);
    zelda64_write_u32(&data[ZMF_OFFSET_CHECK_CODE], (uint32_t) (header->check_code >> 32));
    zelda64_write_u32(&data[ZMF_OFFSET_CHECK_CODE + 0x04], (uint32_t) (header->check_code));
    memcpy(&data[ZMF_OFFSET_PROGRAM], header->program, sizeof header->program);

    return ZELDA64_OK;
}
