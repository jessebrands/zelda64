/*
 * layout_write.c: write a Nintendo 64 Zelda ROM from layout
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

#include <stdio.h>
#include <yaz0/yaz0.h>

#include "allocator.h"
#include "error.h"
#include "layout.h"
#include "rom.h"
#include "zelda64/zelda64.h"

#define CHUNK_SIZE 1024

static zelda64_ssize_t
copy_rom_file(FILE* file, uint32_t const position,
              struct zelda64_rom const* rom, zelda64_index_t const index,
              struct zelda64_error* error) {
    // Get information about this ROM file.
    struct zelda64_stat st;
    if (zelda64_stat(&st, rom, index, error) != ZELDA64_OK) {
        return -1;
    }

    // Empty files are quick and easy, don't need to do anything for them.
    if (st.size == 0) {
        return 0;
    }

    // Seek to the write position that this file belongs to.
    if (fseek(file, position, SEEK_SET) != 0) {
        zelda64_set_errno(error);
        return -1;
    }

    uint32_t bytes_out = 0;
    uint8_t chunk[CHUNK_SIZE];
    while (bytes_out < st.size) {
        uint32_t const available = st.size - bytes_out;
        uint32_t const have = available < CHUNK_SIZE ? available : CHUNK_SIZE;

        if (zelda64_read_storage(chunk, have, rom, index, bytes_out, error) < 0) {
            return -1;
        }

        if (fwrite(chunk, sizeof chunk[0], have, file) != have) {
            zelda64_set_errno(error);
            return -1;
        }

        bytes_out += have;
    }

    return bytes_out;
}

static zelda64_ssize_t
copy_entry(FILE* file, uint32_t const position,
           struct zelda64_layout_entry const* entry,
           struct zelda64_error* error) {
    switch (entry->from_type) {
        case ZELDA64_FROM_ROM: {
            struct zelda64_rom const* rom = entry->from.rom.rom;
            zelda64_index_t const index = entry->from.rom.index;
            return copy_rom_file(file, position, rom, index, error);
        }
    }

    zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    return -1;
}

static zelda64_ssize_t
decompress_rom_file(FILE* file, uint32_t const position,
                    struct zelda64_rom const* rom, zelda64_index_t const index,
                    struct zelda64_allocator const allocator,
                    struct zelda64_error* error) {
    // Get information about this ROM file.
    struct zelda64_stat st;
    if (zelda64_stat(&st, rom, index, error) != ZELDA64_OK) {
        return -1;
    }

    // Empty files are quick and easy, don't need to do anything for them.
    if (st.file_size == 0) {
        return 0;
    }

    // Set up the stream with our allocator.
    struct yaz0_stream stream = {0};
    stream.alloc = allocator.alloc;
    stream.free = allocator.free;
    stream.opaque = allocator.opaque;

    // Initialize the stream for decompression.
    enum yaz0_result decompress_result = yaz0_decompress_init(&stream);
    if (decompress_result != YAZ0_OK) {
        zelda64_set_sys_error(error, ZELDA64_DECOMPRESS_ERROR, decompress_result);
        return -1;
    }

    // Seek to the write position that this file belongs to.
    if (fseek(file, position, SEEK_SET) != 0) {
        zelda64_set_errno(error);
        yaz0_decompress_end(&stream);
        return -1;
    }

    uint8_t in_chunk[CHUNK_SIZE];
    uint8_t out_chunk[CHUNK_SIZE];
    uint32_t bytes_in = 0;
    do {
        // Read data from the ROM into our buffer.
        uint32_t const available = st.size - bytes_in;
        uint32_t const want = available < CHUNK_SIZE ? available : CHUNK_SIZE;
        if (zelda64_read_storage(in_chunk, want, rom, index, bytes_in, error) < 0) {
            yaz0_decompress_end(&stream);
            return -1;
        }

        stream.next_in = in_chunk;
        stream.avail_in = want;
        bytes_in += want;

        enum yaz0_flush const flush = (bytes_in == st.size)
                                  ? YAZ0_FINISH
                                  : YAZ0_NO_FLUSH;

        do {
            stream.next_out = out_chunk;
            stream.avail_out = sizeof out_chunk;

            decompress_result = yaz0_decompress(&stream, flush);
            if (decompress_result < YAZ0_OK) {
                zelda64_set_sys_error(error, ZELDA64_DECOMPRESS_ERROR, decompress_result);
                yaz0_decompress_end(&stream);
                return -1;
            }

            size_t const have = sizeof out_chunk - stream.avail_out;
            if (fwrite(out_chunk, sizeof *out_chunk, have, file) != have) {
                zelda64_set_errno(error);
                yaz0_decompress_end(&stream);
                return -1;
            }
        } while (stream.avail_out == 0 && stream.avail_in != 0);
    } while (decompress_result != YAZ0_STREAM_END);

    yaz0_decompress_end(&stream);
    return (zelda64_ssize_t) stream.total_out;
}

static zelda64_ssize_t
decompress_entry(FILE* file, uint32_t const position,
                 struct zelda64_layout_entry const* entry,
                 struct zelda64_allocator const allocator,
                 struct zelda64_error* error) {
    switch (entry->from_type) {
        case ZELDA64_FROM_ROM: {
            struct zelda64_rom const* rom = entry->from.rom.rom;
            zelda64_index_t const index = entry->from.rom.index;
            return decompress_rom_file(file, position, rom, index, allocator, error);
        }
    }

    zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    return -1;
}

enum zelda64_result
zelda64_write(char const* filename,
              struct zelda64_dmadata_layout const* layout,
              struct zelda64_write_options const* options,
              struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    if (filename == NULL || layout == NULL || options == NULL) {
        return zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    }

    // Get the size of the DMADATA we're making.
    size_t const count = layout->count;

    // We'll need to allocate a DMADATA for ourselves.
    struct zelda64_dmadata* dmadata = zelda64_alloc(layout->allocator, count * sizeof *dmadata);
    if (dmadata == NULL) {
        return zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
    }

    // Create our new ROM.
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        zelda64_set_errno(error);
        zelda64_free(layout->allocator, dmadata);
        return ZELDA64_ERRNO;
    }

    // Enter the writing loop, this is where things get complex.
    uint32_t position = 0;
    for (size_t i = 0; i < count; ++i) {
        struct zelda64_layout_entry const* entry = &layout->entries[i];

        // If this entry has been deleted, we can take this fast path.
        if (entry->operation == ZELDA64_OP_DELETE) {
            dmadata[i] = (struct zelda64_dmadata){
                .vrom_start = entry->vrom_start,
                .vrom_end = entry->vrom_end,
                .rom_start = UINT32_MAX,
                .rom_end = UINT32_MAX,
            };
            continue;
        }

        // Empty files are a special case.
        if (entry->vrom_start == entry->vrom_end) {
            dmadata[i] = (struct zelda64_dmadata){0};
            continue;
        }

        // If we're packing sparsely, we'll write to where the VROM says.
        uint32_t const rom_start = (options->pack == ZELDA64_PACK_SPARSE)
                                       ? entry->vrom_start
                                       : position;

        zelda64_ssize_t bytes_out;
        switch (entry->operation) {
            case ZELDA64_OP_COPY:
                bytes_out = copy_entry(file, rom_start, entry, error);
                break;

            case ZELDA64_OP_DECOMPRESS:
                bytes_out = decompress_entry(file, rom_start, entry, layout->allocator, error);
                break;

            default:
                zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
                bytes_out = -1;
                break;
        }

        if (bytes_out < 0) {
            zelda64_free(layout->allocator, dmadata);
            fclose(file);
            return error->result;
        }

        uint32_t const size = ((uint32_t) bytes_out + 15) & -16u;
        dmadata[i] = (struct zelda64_dmadata){
            .vrom_start = entry->vrom_start,
            .vrom_end = entry->vrom_end,
            .rom_start = rom_start,
            .rom_end = 0,
        };

        position = rom_start + size;
    }

    // Now we must write the DMADATA to the ROM.
    // The DMADATA is at entry 0x0002, so we need to fit exactly there.
    struct zelda64_dmadata const* e_dmadata = &dmadata[0x0002];
    fseek(file, (long) e_dmadata->rom_start, SEEK_SET);

    // TODO: Need DMADATA writing function. :o

    // TODO: Calculate the ROM check code.

    // We're done, clean up!
    zelda64_free(layout->allocator, dmadata);
    fclose(file);
    return ZELDA64_OK;
}
