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

#include "bytes.h"
#include "crc32c.h"
#include "rom.h"

#define MAKEROM_ENTRY            0x0000
#define IPL3_BOOTCODE_START      0x40

#define IPL3_CHECKSUM_END        0x1000
#define IPL3_CHECKSUM_SIZE       (IPL3_CHECKSUM_END - IPL3_BOOTCODE_START)
#define IPL3_CHECKSUM_6105       0x5FFC15B9

#define CHECK_CODE_CHUNK         1024
#define CHECK_CODE_START         0x1000
#define CHECK_CODE_END           0x101000
#define CHECK_CODE_IPL3_END      0x850
#define CHECK_CODE_IPL3_SIZE     (CHECK_CODE_IPL3_END - IPL3_BOOTCODE_START)
#define CHECK_CODE_SEED_6105     0xDF26F436

static zelda64_ssize_t
read_ipl3_bootcode(uint8_t* buffer, size_t const size,
                   struct zelda64_io const* io,
                   struct zelda64_dmadata const* dmadata,
                   size_t const dmadata_count,
                   struct zelda64_error* error) {
    assert(buffer != NULL);
    assert(io != NULL);
    assert(dmadata != NULL);
    assert(dmadata_count > 0);

    struct zelda64_dmadata const* e_makerom = &dmadata[MAKEROM_ENTRY];

    // If this entry is too small, then this is not a valid ROM.
    if (e_makerom->vrom_end - e_makerom->vrom_start < IPL3_CHECKSUM_END) {
        zelda64_set_error(error, ZELDA64_INVALID_ROM);
        return -1;
    }

    return zelda64_io_read_exact(
        io,
        buffer, size,
        IPL3_BOOTCODE_START,
        error
    );
}

static enum zelda64_cic
detect_cic(uint8_t const* ipl3_bootcode, size_t const size) {
    uint32_t const ipl3_checksum = zelda64_crc32c(0, ipl3_bootcode, size);

    // And match it to a known CIC checksum.
    switch (ipl3_checksum) {
        case IPL3_CHECKSUM_6105:
            return ZELDA64_CIC_6105;

        default:
            return ZELDA64_CIC_UNKNOWN;
    }
}

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
    uint8_t ipl3_bootcode[IPL3_CHECKSUM_SIZE];

    zelda64_ssize_t const bytes_in = read_ipl3_bootcode(
        ipl3_bootcode, sizeof ipl3_bootcode,
        &rom->io,
        rom->dmadata,
        rom->dmadata_info.count,
        error
    );

    if (bytes_in < 0) {
        return ZELDA64_CIC_UNKNOWN;
    }

    enum zelda64_cic const cic = detect_cic(ipl3_bootcode, sizeof ipl3_bootcode);
    if (cic == ZELDA64_CIC_UNKNOWN) {
        zelda64_set_error(error, ZELDA64_UNSUPPORTED_CIC);
    }

    return cic;
}

struct check_code_state {
    uint32_t acc[6];
    size_t offset;
    uint8_t const* ipl;
    size_t ipl_size;
};

static uint32_t
check_code_seed(enum zelda64_cic const cic, struct zelda64_error* error) {
    switch (cic) {
        case ZELDA64_CIC_6105:
            return CHECK_CODE_SEED_6105;

        case ZELDA64_CIC_UNKNOWN:
            break;
    }

    zelda64_set_error(error, ZELDA64_UNSUPPORTED_CIC);
    return 0;
}

static enum zelda64_result
check_code_init(enum zelda64_cic const cic,
                uint8_t const* ipl, size_t const ipl_size,
                struct check_code_state* state,
                struct zelda64_error* error) {
    if (state == NULL || ipl == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }
    if (ipl_size < CHECK_CODE_IPL3_SIZE) {
        return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
    }

    // Get initial seed for the CIC-NUS chip.
    uint32_t const seed = check_code_seed(cic, error);
    if (seed == 0) {
        return error->result;
    }

    *state = (struct check_code_state){
        .acc = {seed, seed, seed, seed, seed, seed},
        .offset = CHECK_CODE_START,
        .ipl = ipl,
        .ipl_size = ipl_size
    };

    return ZELDA64_OK;
}

static enum zelda64_result
check_code(struct check_code_state* state,
           uint8_t const* data, size_t const size,
           struct zelda64_error* error) {
    // If our arguments don't make sense, bail.
    if (state == NULL || data == NULL || size % 4 != 0 || state->ipl == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }
    if (state->ipl_size < CHECK_CODE_IPL3_SIZE) {
        return zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
    }

    size_t const start = state->offset;
    size_t i = 0; // Position in current data buffer.
    while (state->offset < CHECK_CODE_END && i + 4 <= size) {
        uint32_t const d = zelda64_read_u32(&data[i]);

        if (state->acc[5] + d < state->acc[5]) {
            state->acc[3]++;
        }

        state->acc[5] += d;
        state->acc[2] ^= d;

        uint32_t const r = zelda64_rot32(d, (d & 0x1F));
        state->acc[4] += r;

        if (state->acc[1] > d) {
            state->acc[1] ^= r;
        } else {
            state->acc[1] ^= state->acc[5] ^ d;
        }

        size_t const makerom_offset = 0x0710 + ((start + i) & 0xFF);
        uint32_t const b = zelda64_read_u32(&state->ipl[makerom_offset]);
        state->acc[0] += b ^ d;

        i += 4;
        state->offset += 4;
    }

    return ZELDA64_OK;
}

static uint64_t
cic_check_code_end(struct check_code_state const* state) {
    return ((uint64_t) (state->acc[5] ^ state->acc[3] ^ state->acc[2]) << 32)
           | ((uint64_t) (state->acc[4] ^ state->acc[1] ^ state->acc[0]));
}

uint64_t
zelda64_calculate_check_code(struct zelda64_io const* io,
                             struct zelda64_dmadata const* dmadata,
                             size_t const dmadata_count,
                             struct zelda64_error* error) {
    if (io == NULL || dmadata == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }
    if (dmadata_count == 0) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return 0;
    }

    uint8_t ipl3_bootcode[IPL3_CHECKSUM_SIZE];

    // To calculate this, we'll need the IPL3 bootcode.
    zelda64_ssize_t const bytes_in = read_ipl3_bootcode(
        ipl3_bootcode, sizeof ipl3_bootcode,
        io,
        dmadata, dmadata_count,
        error
    );

    if (bytes_in < 0) {
        return 0;
    }

    // What is the CIC of this ROM?
    enum zelda64_cic const cic = detect_cic(ipl3_bootcode, sizeof ipl3_bootcode);
    if (cic != ZELDA64_CIC_6105) {
        zelda64_set_error(error, ZELDA64_UNSUPPORTED_CIC);
        return 0;
    }

    // Initialize the check code state.
    struct check_code_state state;
    check_code_init(cic, ipl3_bootcode, sizeof ipl3_bootcode, &state, error);

    // Calculate the check code.
    uint8_t chunk[CHECK_CODE_CHUNK];
    zelda64_offset_t offset = CHECK_CODE_START;
    while (offset < CHECK_CODE_END) {
        size_t const remaining = CHECK_CODE_END - offset;
        size_t const want = remaining < sizeof chunk ? remaining : sizeof chunk;
        if (zelda64_io_read_exact(io, chunk, want, offset, error) < 0) {
            return 0;
        }
        if (check_code(&state, chunk, want, error) != ZELDA64_OK) {
            return 0;
        }
        offset += want;
    }

    // Mix the check code and return the result.
    return cic_check_code_end(&state);
}
