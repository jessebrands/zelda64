/*
 * zmf.h: zelda64 manifest library
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

#ifndef LIBZELDA64_ZMF_H
#define LIBZELDA64_ZMF_H

#include <stddef.h>
#include <stdint.h>

#include "zelda64/zelda64.h"

#define ZELDA64_ZMF_MAKE_VERSION(major, minor) \
((((uint32_t) (major) & 0xFFFFu) << 16) | ((uint32_t) (minor) & 0xFFFFu))
#define ZELDA64_ZMF_VERSION_MAJOR(v) ((uint32_t) (v) >> 16)
#define ZELDA64_ZMF_VERSION_MINOR(v) ((uint32_t) (v) & 0xFFFFu)

#define ZELDA64_ZMF_VERSION ZELDA64_ZMF_MAKE_VERSION(1, 0)

#define ZELDA64_ZMF_HEADER_SIZE       0x40u
#define ZELDA64_ZMF_CHUNK_HEADER_SIZE 0x10u

struct zelda64_zmf_header {
    uint32_t version;
    uint8_t reserved0[8];
    char game_code[4];
    uint8_t game_version;
    uint8_t reserved1[3];
    uint64_t check_code;
    char program[32];
};

struct zelda64_zmf_chunk_header {
    char type[4];
    uint32_t length;
    uint32_t checksum;
    uint8_t reserved[4];
};

/*!
 * \brief Calculates the checksum for a ZMF chunk payload.
 * \param data Chunk payload, may be NULL when size is zero.
 * \param size Payload length in bytes.
 * \return CRC-32/IEEE of the payload; zero for an empty payload.
 */
ZELDA64_API uint32_t
zelda64_zmf_chunk_crc32(uint8_t const* data, size_t size);

/*!
 * \brief Decodes a buffer into a ZMF header.
 * \param header Pointer to a \ref zelda64_zmf_header to receive the decoded data.
 * \param data Buffer holding the data.
 * \param size Size of the buffer in bytes.
 * \return ZELDA64_OK on success.
 */
ZELDA64_API enum zelda64_result
zelda64_zmf_read_header(struct zelda64_zmf_header* header,
                        uint8_t const* data, size_t size);

/*!
 * \brief Encodes a ZMF header into a buffer.
 * \param data Buffer where the header will be written to.
 * \param size Size of the buffer in bytes.
 * \param header Pointer to a \ref zelda64_zmf_header.
 * \return ZELDA64_OK on success.
 */
ZELDA64_API enum zelda64_result
zelda64_zmf_write_header(uint8_t* data, size_t size,
                         struct zelda64_zmf_header const* header);

/*!
 * \brief Decodes a buffer into a ZMF chunk header.
 * \param chunk Pointer to a \ref zelda64_zmf_chunk_header to receive the decoded data.
 * \param data Buffer holding the data.
 * \param size Size of the buffer in bytes.
 * \return ZELDA64_OK on success.
 */
ZELDA64_API enum zelda64_result
zelda64_zmf_read_chunk_header(struct zelda64_zmf_chunk_header* chunk,
                              uint8_t const* data, size_t size);

/*!
 * \brief Encodes a ZMF chunk header into a buffer.
 * \param data Buffer where the header will be written to.
 * \param size Size of the buffer in bytes.
 * \param chunk Pointer to a \ref zelda64_zmf_chunk_header.
 * \return ZELDA64_OK on success.
 */
ZELDA64_API enum zelda64_result
zelda64_zmf_write_chunk_header(uint8_t* data, size_t size,
                               struct zelda64_zmf_chunk_header const* chunk);

#endif //LIBZELDA64_ZMF_H
