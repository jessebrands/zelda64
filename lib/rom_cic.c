/*
 * rom_cic.c: CIC-NUS chip detection
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

#include "crc32c.h"
#include "rom.h"

#define MAKEROM_ENTRY         0x0000
#define IPL3_CHECKSUM_START   0x40
#define IPL3_CHECKSUM_END     0x1000
#define IPL3_CHECKSUM_SIZE    (IPL3_CHECKSUM_END - IPL3_CHECKSUM_START)
#define IPL3_CHECKSUM_6105    0x5FFC15B9

enum zelda64_cic
zelda64_detect_cic(struct zelda64_rom const* rom, struct zelda64_error* error) {
    if (rom == NULL || rom->dmadata == NULL || rom->dmadata_info.count == 0) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return ZELDA64_CIC_UNKNOWN;
    }

    // The region 0x40 ... 0x1000 in the MAKEROM is a static region that
    // contains the IPL3 bootcode. This never changes so we can calculate the
    // checksum of this region. We can then compare it against a set of known
    // values to figure out what CIC that specific IPL bootcode has.
    //
    // For our purposes, we are only interested in CIC-NUS-6105, given that we
    // are a Nintendo 64 Zelda ROM library. :-)
    struct zelda64_dmadata const* e_makerom = &rom->dmadata[MAKEROM_ENTRY];

    // If this entry is too small, then this is not a valid ROM.
    if (e_makerom->vrom_end - e_makerom->vrom_start < IPL3_CHECKSUM_END) {
        zelda64_set_error(error, ZELDA64_INVALID_ROM);
        return ZELDA64_CIC_UNKNOWN;
    }

    // Read the section of the bootcode we care about into memory.
    uint8_t ipl3_bootcode[IPL3_CHECKSUM_SIZE];
    zelda64_ssize_t const bytes_in = zelda64_read_storage(
        ipl3_bootcode, IPL3_CHECKSUM_SIZE,
        rom, MAKEROM_ENTRY,
        IPL3_CHECKSUM_START,
        error
    );

    if (bytes_in < 0) {
        return ZELDA64_CIC_UNKNOWN;
    }
    if (bytes_in != IPL3_CHECKSUM_SIZE) {
        zelda64_set_error(error, ZELDA64_TRUNCATED);
        return ZELDA64_CIC_UNKNOWN;
    }

    // Calculate the CRC-32C of this region.
    uint32_t const ipl3_checksum = zelda64_crc32c(0, ipl3_bootcode, sizeof ipl3_bootcode);

    // And match it to a known CIC checksum.
    switch (ipl3_checksum) {
        case IPL3_CHECKSUM_6105:
            return ZELDA64_CIC_6105;

        default:
            zelda64_set_error(error, ZELDA64_UNSUPPORTED_CIC);
            return ZELDA64_CIC_UNKNOWN;
    }
}
