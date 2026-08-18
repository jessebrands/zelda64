/*
 * layout_bound.c: output size calculations
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

#include <yaz0/yaz0.h>

#include "bytes.h"
#include "layout.h"
#include "zelda64/zelda64.h"

static uint32_t
copy_bound(struct zelda64_layout_entry const* entry,
           struct zelda64_error* error) {
    switch (entry->from_type) {
        case ZELDA64_FROM_ROM: {
            struct zelda64_stat st;
            zelda64_stat(&st, entry->from.rom.rom, entry->from.rom.index, error);
            if (error->result != ZELDA64_OK) {
                return 0;
            }
            return st.size;
        }

        default: {
            zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
            return 0;
        }
    }
}

static uint32_t
compress_bound(struct zelda64_layout_entry const* entry,
               struct zelda64_error* error) {
    uint32_t const uncompressed_size = copy_bound(entry, error);
    if (error->result != ZELDA64_OK) {
        return 0;
    }
    size_t const compressed_size = yaz0_compress_bound(uncompressed_size);
    if (compressed_size >= UINT32_MAX) {
        zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
        return 0;
    }
    return (uint32_t) compressed_size;
}

static uint32_t
decompress_bound(struct zelda64_layout_entry const* entry) {
    return entry->vrom_end - entry->vrom_start;
}

uint32_t
zelda64_layout_entry_bound(struct zelda64_layout_entry const* entry,
                           struct zelda64_error* error) {
    switch (entry->operation) {
        case ZELDA64_OP_COPY: {
            return copy_bound(entry, error);
        }

        case ZELDA64_OP_COMPRESS: {
            return compress_bound(entry, error);
        }

        case ZELDA64_OP_DECOMPRESS: {
            return decompress_bound(entry);
        }

        case ZELDA64_OP_DELETE: {
            break;
        }
    }
    return 0;
}

static uint32_t
zelda64_layout_max(struct zelda64_dmadata_layout const* layout,
                   struct zelda64_error* error) {
    uint32_t biggest = 0;
    for (size_t i = 0; i < layout->count; ++i) {
        struct zelda64_layout_entry const* entry = &layout->entries[i];
        if (entry->operation == ZELDA64_OP_DELETE) {
            continue;
        }
        if (entry->vrom_start == entry->vrom_end) {
            continue;
        }

        uint32_t const start = entry->vrom_start;
        uint32_t const size = zelda64_layout_entry_bound(entry, error);
        if (error->result != ZELDA64_OK) {
            return 0;
        }

        uint32_t const aligned_size = zelda64_align16(size);
        if (start > UINT32_MAX - aligned_size) {
            zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
            return 0;
        }

        uint32_t const end = start + aligned_size;
        if (end > biggest) {
            biggest = end;
        }
    }
    return biggest;
}

uint32_t
zelda64_write_bound(struct zelda64_dmadata_layout const* layout,
                    struct zelda64_write_options const* options,
                    struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }

    // Sparse is just the maximum value.
    if (options->pack == ZELDA64_PACK_SPARSE) {
        return zelda64_layout_max(layout, error);
    }

    uint32_t bound = 0;
    for (size_t i = 0; i < layout->count; ++i) {
        struct zelda64_layout_entry const* entry = &layout->entries[i];
        if (entry->operation == ZELDA64_OP_DELETE) {
            continue;
        }
        if (entry->vrom_start == entry->vrom_end) {
            continue;
        }

        uint32_t const size = zelda64_layout_entry_bound(entry, error);
        if (error->result != ZELDA64_OK) {
            return 0;
        }

        uint32_t const aligned_size = zelda64_align16(size);
        if (bound > UINT32_MAX - aligned_size) {
            zelda64_set_error(error, ZELDA64_OUT_OF_RANGE);
            return 0;
        }

        bound += aligned_size;
        if (error->result != ZELDA64_OK) {
            return 0;
        }
    }

    // Are we padding?
    if (options->pad != ZELDA64_PAD_NONE) {
        bound = zelda64_ceil_pow2(bound);
    }

    return bound;
}
