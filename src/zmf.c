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
#include "crc32.h"

#define ZMF_OFFSET_MAGIC        0x00
#define ZMF_OFFSET_VERSION      0x04
#define ZMF_OFFSET_RESERVED0    0x08
#define ZMF_OFFSET_GAME_CODE    0x10
#define ZMF_OFFSET_GAME_VERSION 0x14
#define ZMF_OFFSET_RESERVED1    0x15
#define ZMF_OFFSET_CHECK_CODE   0x18
#define ZMF_OFFSET_PROGRAM      0x20

#define ZMF_CHUNK_OFFSET_TYPE     0x00
#define ZMF_CHUNK_OFFSET_LENGTH   0x04
#define ZMF_CHUNK_OFFSET_CRC32    0x08
#define ZMF_CHUNK_OFFSET_RESERVED 0x0C

static char const zmf_magic[4] = {'Z', '6', '4', 'M'};

uint32_t
zelda64_zmf_chunk_checksum(uint32_t const checksum, uint8_t const* data, size_t const size) {
    if (data == NULL || size == 0) {
        return 0;
    }
    return zelda64_crc32(checksum, data, size);
}

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

enum zelda64_result
zelda64_zmf_read_chunk_header(struct zelda64_zmf_chunk_header* chunk,
                              uint8_t const* data, size_t size) {
    if (chunk == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_ZMF_CHUNK_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    memcpy(chunk->type, &data[ZMF_CHUNK_OFFSET_TYPE], sizeof chunk->type);
    chunk->length = zelda64_read_u32(&data[ZMF_CHUNK_OFFSET_LENGTH]);
    chunk->checksum = zelda64_read_u32(&data[ZMF_CHUNK_OFFSET_CRC32]);
    memcpy(chunk->reserved, &data[ZMF_CHUNK_OFFSET_RESERVED], sizeof chunk->reserved);

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_zmf_write_chunk_header(uint8_t* data, size_t size,
                               struct zelda64_zmf_chunk_header const* chunk) {
    if (chunk == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_ZMF_CHUNK_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    memcpy(&data[ZMF_CHUNK_OFFSET_TYPE], chunk->type, sizeof chunk->type);
    zelda64_write_u32(&data[ZMF_CHUNK_OFFSET_LENGTH], chunk->length);
    zelda64_write_u32(&data[ZMF_CHUNK_OFFSET_CRC32], chunk->checksum);
    memcpy(&data[ZMF_CHUNK_OFFSET_RESERVED], chunk->reserved, sizeof chunk->reserved);

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_zmf_chunk_extent(struct zelda64_zmf_chunk_header const* chunk,
                         size_t const offset, size_t const total_size,
                         size_t* payload_offset, size_t* next_offset) {
    if (chunk == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (offset > total_size || total_size - offset < ZELDA64_ZMF_CHUNK_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }

    size_t const payload = offset + ZELDA64_ZMF_CHUNK_HEADER_SIZE;
    size_t const padded = ((size_t) chunk->length + 15u) & -16;

    // total_size - payload cannot wrap: the check above proved payload <= total_size.
    if (padded > total_size - payload) {
        return ZELDA64_BAD_MANIFEST;
    }

    if (payload_offset != NULL) {
        *payload_offset = payload;
    }
    if (next_offset != NULL) {
        *next_offset = payload + padded;
    }

    return ZELDA64_OK;
}

size_t
zelda64_zmf_copy_list_size(size_t const count) {
    return (count + 7) / 8;
}

bool
zelda64_zmf_copy_list_test(uint8_t const* data, size_t const size, size_t const index) {
    if (data == NULL || index / 8u >= size) {
        return false;
    }

    return (data[index / 8u] & (0x80u >> (index % 8u))) != 0u;
}

enum zelda64_result
zelda64_zmf_copy_list_set(uint8_t* data, size_t const size, size_t const index) {
    if (data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (index / 8u >= size) {
        return ZELDA64_OUT_OF_RANGE;
    }

    data[index / 8u] |= (uint8_t) (0x80u >> (index % 8u));
    return ZELDA64_OK;
}
