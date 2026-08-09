/*
 * zelda64.h: Nintendo 64 Zelda ROM manipulation library
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

#ifndef LIBZELDA64_ZELDA64_H
#define LIBZELDA64_ZELDA64_H

#include <stddef.h>
#include <stdint.h>

#include "zelda64/config.h"

#if defined _WIN32 || defined __CYGWIN__
#  if defined ZELDA64_STATIC
#    define ZELDA64_API
#  elif defined ZELDA64_SHARED
#    define ZELDA64_API __declspec(dllexport)
#  else
#    define ZELDA64_API __declspec(dllimport)
#  endif
#elif defined __GNUC__ && !defined ZELDA64_STATIC
#  define ZELDA64_API __attribute__((visibility("default")))
#else
#  define ZELDA64_API
#endif

#if defined __cplusplus
extern "C" {
#endif

#define ZELDA64_ROM_HEADER_SIZE     0x40u
#define ZELDA64_DMA_ENTRY_SIZE      0x10u

enum zelda64_result {
    ZELDA64_OK = 0,
    ZELDA64_INVALID_PARAMETER = -1,
    ZELDA64_OUT_OF_RANGE = -2,
    ZELDA64_NO_DMADATA = -3,
    ZELDA64_MEMORY_ERROR = -4,
    ZELDA64_COMPRESSION_ERROR = -5,
    ZELDA64_DECOMPRESSION_ERROR = -6,
    ZELDA64_IO_ERROR = -100,
};

enum zelda64_compression_level {
    ZELDA64_DEFAULT_COMPRESSION = -1,
    ZELDA64_NO_COMPRESSION = 0,
    ZELDA64_BEST_COMPRESSION = 9,
};

enum zelda64_cic {
    ZELDA64_CIC_UNKNOWN = 0,
    ZELDA64_CIC_6101 = 6101,
    ZELDA64_CIC_6102 = 6102,
    ZELDA64_CIC_6103 = 6103,
    ZELDA64_CIC_6105 = 6105,
    ZELDA64_CIC_6106 = 6106,
};

enum zelda64_dma_kind {
    ZELDA64_DMA_EMPTY = 0,
    ZELDA64_DMA_DELETED,
    ZELDA64_DMA_UNCOMPRESSED,
    ZELDA64_DMA_COMPRESSED,
};

struct zelda64_rom_header {
    uint8_t reserved0; // 0x00  0x80 on all commercial ROMs
    uint8_t pi_config[3]; // 0x01  PI BSD DOM1 RLS/PGS/PWD/LAT
    uint32_t clock_rate; // 0x04
    uint32_t boot_address; // 0x08  not the entry point on 6103/6106
    uint32_t libultra_version; // 0x0C
    uint64_t check_code; // 0x10  commonly called CRC1/CRC2
    uint8_t reserved1[8]; // 0x18
    char title[20]; // 0x20  20 bytes, padded (NOT NUL-terminated)
    uint8_t reserved2[7]; // 0x34
    char game_code[4]; // 0x3B  4 bytes (NOT NUL-terminated)
    uint8_t version; // 0x3F
};

struct zelda64_rom_info {
    struct zelda64_rom_header header;
    uint32_t ipl_checksum;
    enum zelda64_cic cic;
    uint32_t entrypoint;
};

struct zelda64_check_code_state {
    enum zelda64_cic cic;
    uint32_t acc[6];
    size_t offset;
    uint8_t const* ipl;
    size_t ipl_size;
};

struct zelda64_dmadata_info {
    uint32_t offset; // physical offset of DMADATA in ROM
    uint32_t size; // size in bytes
    size_t count; // count of entries in DMADATA
};

struct zelda64_dma_entry {
    uint32_t vrom_start;
    uint32_t vrom_end;
    uint32_t rom_start;
    uint32_t rom_end;
};

ZELDA64_API char const* zelda64_result_string(enum zelda64_result result);

ZELDA64_API char const*
zelda64_version_string(void);

ZELDA64_API enum zelda64_result
zelda64_read_rom_header(struct zelda64_rom_header* header, uint8_t const* data, size_t size);

ZELDA64_API enum zelda64_result
zelda64_write_rom_header(uint8_t* data, size_t size, struct zelda64_rom_header const* header);

/*
 * Reads and decodes the Nintendo 64 IPL section of a ROM.
 */
ZELDA64_API enum zelda64_result
zelda64_read_rom_info(struct zelda64_rom_info* info, uint8_t const* data, size_t size);

/*!
 * \brief Sets initial state for the CIC check code algorithm.
 * \param state Check code algorithm state.
 * \return ZELDA64_OK on success.
 * \note This is part of the streaming interface.
 * \see zelda64_cic_check_code, zelda64_cic_check_code_end
 */
ZELDA64_API enum zelda64_result
zelda64_cic_check_code_init(enum zelda64_cic cic,
                            uint8_t const* ipl, size_t ipl_size,
                            struct zelda64_check_code_state* state);

/*!
 * \brief Calculates state for the CIC check code algorithm.
 * \param state Check code algorithm state.
 * \param data Data to operate on.
 * \param size Size of data in bytes.
 * \return ZELDA64_OK on success.
 * \note This is part of the streaming API.
 * \see zelda64_cic_check_code_init, zelda64_cic_check_code_end
 */
ZELDA64_API enum zelda64_result
zelda64_cic_check_code(struct zelda64_check_code_state* state,
                       uint8_t const* data, size_t size);

/*!
 * \brief Mixes the calculation state into a final check code.
 * \param state Check code algorithm state.
 * \return Check code for the data.
 * \note This is part of the streaming API.
 * \see zelda64_cic_check_code_init, zelda64_cic_check_code
 */
ZELDA64_API uint64_t
zelda64_cic_check_code_end(struct zelda64_check_code_state const* state);

ZELDA64_API char const*
zelda64_cic_name(enum zelda64_cic cic);

/*!
 * \brief Calculates the CIC-NUS check code for a ROM.
 * \param rom The ROM.
 * \param rom_size Size of the ROM.
 * \param check_code A pointer that receives the calculated check code.
 * \return \ref ZELDA64_OK on success.
 */
ZELDA64_API enum zelda64_result
zelda64_rom_check_code(uint8_t const* rom, size_t rom_size, uint64_t* check_code);

/*!
 * \brief Searches for the start of DMADATA in a buffer.
 *
 * \param data Buffer to search in.
 * \param size Size of the buffer in bytes.
 * \param offset A pointer to an offset in the buffer to start searching for.
 *               The pointer will be set to the last search position.
 * \return ZELDA64_OK if a potential match is found.
 * \note The buffer start must be aligned to 16 bytes in the ROM.
 * \note A buffer can have multiple candidates, use
 *       \ref zelda64_read_dmadata_info to check if the found offset is truly
 *       DMADATA.
 */
ZELDA64_API enum zelda64_result
zelda64_find_dmadata_start(uint8_t const* data, size_t size, size_t* offset);

/*!
 * \brief Read the DMADATA info from a buffer.
 *
 * This function attempts to read information about DMADATA from the buffer
 * given at data. It does this by checking if the first 3 DMA entries in the
 * buffer match an expected signature.
 *
 * If the signature is a match, the function will perform a series of checks
 * to confirm that the data is DMADATA.
 *
 * \param info A pointer to a \ref zelda64_dmadata_info that receives
 *             information about the DMA table.
 * \param offset The absolute offset in the ROM where data is.
 * \param rom_size Size of the ROM in bytes.
 * \param data Pointer to the buffer to read from.
 * \param size Size of the buffer at data.
 * \return \ref ZELDA64_OK if a DMA table could be read from data.
 * \note This function always check from the beginning of data, offset is used
 *       to check the signature of the DMADATA and is not an offset into data.
 */
ZELDA64_API enum zelda64_result
zelda64_read_dmadata_info(struct zelda64_dmadata_info* info, size_t offset, size_t rom_size,
                          uint8_t const* data, size_t size);

/*!
 * \brief Finds DMADATA info inside a ROM.
 *
 * This function tries to locate the DMADATA inside a ROM and returns
 * information about the location and size of DMADATA when found. This function
 * operates on whole ROMs.
 *
 * \param info A pointer to a \ref zelda64_dmadata_info that receives
 * \param rom Buffer containing the ROM data.
 * \param rom_size Size of the ROM data.
 * \return ZELDA64_OK if found.
 */
ZELDA64_API enum zelda64_result
zelda64_find_dmadata(struct zelda64_dmadata_info* info, uint8_t const* rom, size_t rom_size);

ZELDA64_API enum zelda64_result
zelda64_read_dmadata(struct zelda64_dma_entry* entries, size_t count,
                     uint8_t const* data, size_t size);

ZELDA64_API enum zelda64_result
zelda64_write_dmadata(uint8_t* data, size_t size,
                      struct zelda64_dma_entry const* entries, size_t count);

ZELDA64_API enum zelda64_dma_kind
zelda64_dma_entry_kind(struct zelda64_dma_entry const* entry);

ZELDA64_API enum zelda64_result
zelda64_dma_entry_extent(struct zelda64_dma_entry const* entry,
                         uint32_t* offset, uint32_t* size);

#if defined __cplusplus
}
#endif

#endif //LIBZELDA64_ZELDA64_H
