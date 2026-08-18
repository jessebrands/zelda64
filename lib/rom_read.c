/*
 * rom_read.c: ROM file reading functions
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

#include "error.h"
#include "rom.h"
#include "zelda64/zelda64.h"

#define CHUNK_SIZE 1024

static size_t
read_bytes(void* buffer, size_t const size,
           struct zelda64_rom const* rom, uint32_t const offset,
           struct zelda64_stat const* st,
           struct zelda64_error* error) {
    // Reading past the end of the file is not an error,
    // we just don't read any bytes at all.
    if (offset >= st->size) {
        return 0;
    }

    // Set up the read.
    size_t const available = st->size - offset;
    size_t const want = available < size ? available : size;
    return zelda64_io_read(
        &rom->io,
        buffer, want,
        (zelda64_offset_t) st->offset + offset,
        error
    );
}

static size_t
decompress(void* buffer, uint32_t const file_size,
           struct zelda64_rom const* rom,
           struct zelda64_stat const* st,
           struct zelda64_error* error) {
    // Initialize the stream with our allocator.
    struct yaz0_stream stream = {0};
    stream.alloc = rom->allocator.alloc;
    stream.free = rom->allocator.free;
    stream.opaque = rom->allocator.opaque;

    enum yaz0_result decompress_result = yaz0_decompress_init(&stream);
    if (decompress_result != YAZ0_OK) {
        zelda64_set_sys_error(error, ZELDA64_DECOMPRESS_ERROR, decompress_result);
        return 0;
    }

    // We have a full buffer available, so use it.
    stream.next_out = buffer;
    stream.avail_out = file_size;

    uint32_t position = 0;
    uint8_t chunk[CHUNK_SIZE];

    do {
        // Get the next chunk of bytes from the ROM.
        uint32_t const remaining = st->size - position;
        uint32_t const want = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

        size_t const bytes_in = read_bytes(chunk, want, rom, position, st, error);
        if (ZELDA64_FAILED(error)) {
            yaz0_decompress_end(&stream);
            return 0;
        }

        // Set up the stream with the newly read bytes.
        stream.avail_in = bytes_in;
        stream.next_in = chunk;
        position += (uint32_t) bytes_in;

        // If no more bytes are coming, the compressor needs to finish work.
        enum yaz0_flush const flush = (position == st->size)
                                          ? YAZ0_FINISH
                                          : YAZ0_NO_FLUSH;

        // Perform decompression.
        decompress_result = yaz0_decompress(&stream, flush);
        if (decompress_result < YAZ0_OK) {
            zelda64_set_sys_error(error, ZELDA64_DECOMPRESS_ERROR, decompress_result);
            yaz0_decompress_end(&stream);
            return 0;
        }
    } while (decompress_result != YAZ0_STREAM_END);

    // Clean up.
    yaz0_decompress_end(&stream);
    return stream.total_out;
}

size_t
zelda64_read_storage(void* buffer, size_t const size,
                     struct zelda64_rom const* rom,
                     zelda64_index_t const index, uint32_t const offset,
                     struct zelda64_error* error) {
    struct zelda64_error local_error = {0};
    if (error == NULL) {
        error = &local_error;
    }

    // Can't read from nothing, nor can we write to nothing.
    if (rom == NULL || (buffer == NULL && size > 0)) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    // Get information about this file.
    struct zelda64_stat st;
    zelda64_stat(&st, rom, index, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    return read_bytes(buffer, size, rom, offset, &st, error);
}

size_t
zelda64_read_file(void* buffer, size_t const size,
                  struct zelda64_rom const* rom, zelda64_index_t const index,
                  struct zelda64_error* error) {
    struct zelda64_error local_error;
    if (error == NULL) {
        error = &local_error;
    }

    // Can't read from nothing, nor can we write to nothing.
    if (rom == NULL || (buffer == NULL && size > 0)) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    // Get information about this file.
    struct zelda64_stat st;
    zelda64_stat(&st, rom, index, error);
    if (ZELDA64_FAILED(error)) {
        return 0;
    }

    // If the buffer is smaller than the file, we have a problem.
    if (size < (size_t) st.file_size) {
        zelda64_set_error(error, ZELDA64_INVALID_PARAMETER);
        return 0;
    }

    // If this is a compressed file, we're going to have to decompress it.
    if (st.method == ZELDA64_METHOD_YAZ0) {
        return decompress(buffer, st.file_size, rom, &st, error);
    }

    // if this is a regular file, we can just dump the bytes.
    return read_bytes(buffer, size, rom, 0, &st, error);
}
