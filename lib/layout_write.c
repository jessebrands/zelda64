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

#include <limits.h>
#include <stdio.h>
#include <yaz0/yaz0.h>

#include "allocator.h"
#include "bytes.h"
#include "error.h"
#include "layout.h"
#include "rom.h"
#include "zelda64/zelda64.h"

#define CHUNK_SIZE 1024

static size_t
copy_rom_file(struct zelda64_io* out_file, uint32_t const position,
              struct zelda64_rom const* in_rom, zelda64_index_t const index,
              struct zelda64_error* error) {
    // Get information about this ROM file.
    struct zelda64_stat st;
    zelda64_stat(&st, in_rom, index, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    // Empty files are quick and easy, don't need to do anything for them.
    if (st.size == 0) {
        return 0;
    }

    size_t bytes_out = 0;
    uint8_t chunk[CHUNK_SIZE];
    while (bytes_out < st.size) {
        size_t const available = st.size - bytes_out;
        size_t const have = available < CHUNK_SIZE ? available : CHUNK_SIZE;
        zelda64_read_storage(chunk, have, in_rom, index, (uint32_t) bytes_out, error);
        if (ZELDA64_FAILED(error)) {
            return 0;
        }

        size_t const offset = position + bytes_out;
        bytes_out += zelda64_io_write(out_file, chunk, have, offset, error);
        if (ZELDA64_FAILED(error)) {
            return 0;
        }
    }

    return bytes_out;
}

static size_t
copy_entry(struct zelda64_io* out_file, uint32_t const position,
           struct zelda64_layout_entry const* entry,
           struct zelda64_error* error) {
    switch (entry->from_type) {
        case ZELDA64_FROM_ROM: {
            struct zelda64_rom const* in_rom = entry->from.rom.rom;
            zelda64_index_t const index = entry->from.rom.index;
            return copy_rom_file(out_file, position, in_rom, index, error);
        }
    }

    zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    return 0;
}

static size_t
compress_rom_file(struct zelda64_io* out_file, uint32_t const position,
                  struct zelda64_rom const* in_rom, zelda64_index_t const index,
                  struct zelda64_allocator const allocator,
                  struct zelda64_error* error) {
    // Get information about this ROM file.
    struct zelda64_stat st;
    zelda64_stat(&st, in_rom, index, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
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

    // Initialize the stream for compression.
    enum yaz0_result compress_result = yaz0_compress_init(&stream, YAZ0_DEFAULT_COMPRESSION, st.size);
    if (compress_result != YAZ0_OK) {
        zelda64_set_sys_error(error, ZELDA64_COMPRESS_ERROR, compress_result);
        return 0;
    }

    uint8_t in_chunk[CHUNK_SIZE];
    uint8_t out_chunk[CHUNK_SIZE];
    uint32_t bytes_in = 0;
    size_t bytes_out = 0;
    do {
        // Read data from the ROM into our buffer.
        uint32_t const available = st.size - bytes_in;
        uint32_t const want = available < CHUNK_SIZE ? available : CHUNK_SIZE;
        zelda64_read_storage(in_chunk, want, in_rom, index, bytes_in, error);
        if (ZELDA64_FAILED(error)) {
            yaz0_compress_end(&stream);
            return 0;
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

            compress_result = yaz0_compress(&stream, flush);
            if (compress_result < YAZ0_OK) {
                zelda64_set_sys_error(error, ZELDA64_COMPRESS_ERROR, compress_result);
                yaz0_compress_end(&stream);
                return 0;
            }

            size_t const have = sizeof out_chunk - stream.avail_out;
            size_t const offset = position + bytes_out;
            bytes_out += zelda64_io_write(out_file, out_chunk, have, offset, error);
            if (ZELDA64_FAILED(error)) {
                yaz0_compress_end(&stream);
                return 0;
            }
        } while (stream.avail_out == 0 && stream.avail_in != 0);
    } while (compress_result != YAZ0_STREAM_END);

    yaz0_compress_end(&stream);
    return stream.total_out;
}

static size_t
compress_entry(struct zelda64_io* out_file, uint32_t const position,
               struct zelda64_layout_entry const* entry,
               struct zelda64_allocator const allocator,
               struct zelda64_error* error) {
    switch (entry->from_type) {
        case ZELDA64_FROM_ROM: {
            struct zelda64_rom const* in_rom = entry->from.rom.rom;
            zelda64_index_t const index = entry->from.rom.index;
            return compress_rom_file(out_file, position, in_rom, index, allocator, error);
        }
    }

    zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    return 0;
}

static size_t
decompress_rom_file(struct zelda64_io* out_file, uint32_t const position,
                    struct zelda64_rom const* in_rom, zelda64_index_t const index,
                    struct zelda64_allocator const allocator,
                    struct zelda64_error* error) {
    // Get information about this ROM file.
    struct zelda64_stat st;
    zelda64_stat(&st, in_rom, index, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
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
        return 0;
    }

    uint8_t in_chunk[CHUNK_SIZE];
    uint8_t out_chunk[CHUNK_SIZE];
    uint32_t bytes_in = 0;
    size_t bytes_out = 0;
    do {
        // Read data from the ROM into our buffer.
        uint32_t const available = st.size - bytes_in;
        uint32_t const want = available < CHUNK_SIZE ? available : CHUNK_SIZE;
        zelda64_read_storage(in_chunk, want, in_rom, index, bytes_in, error);
        if (ZELDA64_FAILED(error)) {
            yaz0_decompress_end(&stream);
            return 0;
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
                return 0;
            }

            size_t const have = sizeof out_chunk - stream.avail_out;
            size_t const offset = position + bytes_out;
            bytes_out += zelda64_io_write(out_file, out_chunk, have, offset, error);
            if (ZELDA64_FAILED(error)) {
                yaz0_decompress_end(&stream);
                return 0;
            }
        } while (stream.avail_out == 0 && stream.avail_in != 0);
    } while (decompress_result != YAZ0_STREAM_END);

    yaz0_decompress_end(&stream);
    return stream.total_out;
}

static size_t
decompress_entry(struct zelda64_io* out_file, uint32_t const position,
                 struct zelda64_layout_entry const* entry,
                 struct zelda64_allocator const allocator,
                 struct zelda64_error* error) {
    switch (entry->from_type) {
        case ZELDA64_FROM_ROM: {
            struct zelda64_rom const* rom = entry->from.rom.rom;
            zelda64_index_t const index = entry->from.rom.index;
            return decompress_rom_file(out_file, position, rom, index, allocator, error);
        }
    }

    zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
    return 0;
}

#define WRITE_COUNT 128

static void
write_dmadata(struct zelda64_io* out_file, uint32_t const position,
              struct zelda64_dmadata const* dmadata, size_t const count,
              struct zelda64_error* error) {
    zelda64_offset_t offset = position;
    uint8_t chunk[ZELDA64_DMA_ENTRY_SIZE * WRITE_COUNT];
    for (size_t i = 0; i < count; i += WRITE_COUNT) {
        size_t const remaining = count - i;
        size_t const have = remaining < WRITE_COUNT ? remaining : WRITE_COUNT;

        for (size_t j = 0; j < have; ++j) {
            uint8_t* const p = &chunk[j * ZELDA64_DMA_ENTRY_SIZE];
            zelda64_write_u32(&p[0], dmadata[i + j].vrom_start);
            zelda64_write_u32(&p[4], dmadata[i + j].vrom_end);
            zelda64_write_u32(&p[8], dmadata[i + j].rom_start);
            zelda64_write_u32(&p[12], dmadata[i + j].rom_end);
        }

        size_t const bytes_out = have * ZELDA64_DMA_ENTRY_SIZE;
        offset += zelda64_io_write(out_file, chunk, bytes_out, offset, error);
        if (ZELDA64_FAILED(error)) {
            return;
        }
    }
}

static size_t
pad_file(struct zelda64_io* out_file, uint32_t position, enum zelda64_pad const pad,
         struct zelda64_error* error) {
    uint32_t const end = zelda64_ceil_pow2(position);
    uint8_t chunk[CHUNK_SIZE];
    size_t bytes_out = 0;
    while ((position + bytes_out) < end) {
        uint32_t const offset = position + (uint32_t) bytes_out;
        uint32_t const available = end - offset;
        uint32_t const want = available < CHUNK_SIZE ? available : CHUNK_SIZE;

        // Prepare a chunk of bytes.
        for (uint32_t i = 0; i < want; ++i) {
            chunk[i] = (pad == ZELDA64_PAD_RAMP)
                           ? (uint8_t) ((offset + i) & 0xFF)
                           : 0;
        }

        bytes_out += zelda64_io_write(out_file, chunk, want, offset, error);
        if (ZELDA64_FAILED(error)) {
            return 0;
        }
    }

    return bytes_out;
}

static size_t
write_rom(struct zelda64_io* out_rom,
          struct zelda64_dmadata_layout const* layout,
          struct zelda64_write_options const* options,
          struct zelda64_error* error) {
    // Get the size of the DMADATA we're making.
    size_t const count = layout->count;

    // We'll need to allocate a DMADATA for ourselves.
    struct zelda64_dmadata* dmadata = zelda64_alloc(layout->allocator, count * sizeof *dmadata);
    if (dmadata == NULL) {
        zelda64_set_error(error, ZELDA64_MEMORY_ERROR);
        return 0;
    }

    // Enter the writing loop, this is where things get complex.
    uint32_t position = 0;
    uint32_t rom_size = 0;
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

        size_t bytes_out = 0;
        switch (entry->operation) {
            case ZELDA64_OP_COPY:
                bytes_out = copy_entry(out_rom, rom_start, entry, error);
                break;

            case ZELDA64_OP_COMPRESS:
                bytes_out = compress_entry(out_rom, rom_start, entry, layout->allocator, error);
                break;

            case ZELDA64_OP_DECOMPRESS:
                bytes_out = decompress_entry(out_rom, rom_start, entry, layout->allocator, error);
                break;

            default:
                zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
                break;
        }

        if (ZELDA64_FAILED(error)) {
            zelda64_free(layout->allocator, dmadata);
            return 0;
        }

        uint32_t const size = zelda64_align16((uint32_t) bytes_out);
        dmadata[i] = (struct zelda64_dmadata){
            .vrom_start = entry->vrom_start,
            .vrom_end = entry->vrom_end,
            .rom_start = rom_start,
            .rom_end = (entry->operation == ZELDA64_OP_COMPRESS) ? rom_start + size : 0,
        };

        // Update the cursor, and check if the ROM has gotten bigger since.
        position = rom_start + size;
        if (position > rom_size) {
            rom_size = position;
        }
    }

    // Pad out the rest of the bytes in the ROM, if that was requested.
    if (options->pad != ZELDA64_PAD_NONE) {
        rom_size += pad_file(out_rom, rom_size, options->pad, error);
        if (ZELDA64_FAILED(error)) {
            zelda64_free(layout->allocator, dmadata);
            return 0;
        }
    }

    // Now we must write the DMADATA to the ROM.
    // The DMADATA is at entry 0x0002, so we need to fit exactly there.
    struct zelda64_dmadata const* e_dmadata = &dmadata[0x0002];
    write_dmadata(out_rom, e_dmadata->vrom_start, dmadata, count, error);
    if (ZELDA64_FAILED(error)) {
        zelda64_free(layout->allocator, dmadata);
        return 0;
    }

    // Calculate the ROM check code.
    uint64_t const check_code = zelda64_calculate_check_code(out_rom, dmadata, count, error);
    if (ZELDA64_FAILED(error)) {
        zelda64_free(layout->allocator, dmadata);
        return 0;
    }

    uint8_t check_code_buffer[8];
    zelda64_write_u64(check_code_buffer, check_code);

    // And write it to the ROM.
    zelda64_io_write(out_rom, check_code_buffer, sizeof check_code_buffer, 0x10, error);
    zelda64_free(layout->allocator, dmadata);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    return rom_size;
}

size_t
zelda64_write(char const* filename,
              struct zelda64_dmadata_layout const* layout,
              struct zelda64_write_options const* options,
              struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }
    zelda64_clear_error(error);

    if (filename == NULL || layout == NULL || options == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    struct zelda64_io out_file = {0};
    zelda64_io_fopen(&out_file, filename, layout->allocator, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    size_t const bytes_out = write_rom(&out_file, layout, options, error);
    zelda64_io_close(&out_file);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    return bytes_out;
}

size_t
zelda64_write_buffer(uint8_t* data, size_t const size,
                     struct zelda64_dmadata_layout const* layout,
                     struct zelda64_write_options const* options,
                     struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }
    zelda64_clear_error(error);

    if ((data == NULL && size > 0) || layout == NULL || options == NULL) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    struct zelda64_io out_buffer = {0};
    zelda64_io_from_buffer(&out_buffer, data, size, layout->allocator, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    size_t const bytes_out = write_rom(&out_buffer, layout, options, error);
    zelda64_io_close(&out_buffer);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    return bytes_out;
}
