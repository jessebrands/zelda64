/*
 * rom.c: ROM file operations
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of zelda64.
 *
 * zelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * zelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <yaz0/yaz0.h>

#include "rom.h"
#include "log.h"

#define CHUNK_COUNT 128
#define CHUNK_SIZE  (CHUNK_COUNT * ZELDA64_DMA_ENTRY_SIZE)

static enum zelda64_result
get_dma_info(struct zelda64_rom* rom) {
    size_t offset = 0;
    while (true) {
        // Find the next read position.
        if (fseek(rom->file, (long) offset, SEEK_SET) != 0) {
            return ZELDA64_IO_ERROR;
        }

        // Read the next chunk of data.
        uint8_t chunk[1024];
        if (fread(chunk, sizeof chunk, 1, rom->file) != 1) {
            return ZELDA64_IO_ERROR;
        }

        size_t seek_pos = 0;
        while (zelda64_find_dmadata_start(chunk, sizeof chunk, &seek_pos) == ZELDA64_OK) {
            // We found a position to look at, is this DMADATA?
            size_t dma_start = offset + seek_pos;
            if (fseek(rom->file, (long) dma_start, SEEK_SET) != 0) {
                return ZELDA64_IO_ERROR;
            }

            uint8_t entries[3 * ZELDA64_DMA_ENTRY_SIZE];
            if (fread(entries, ZELDA64_DMA_ENTRY_SIZE, 3, rom->file) != 3) {
                return ZELDA64_IO_ERROR;
            }

            enum zelda64_result const result = zelda64_read_dmadata_info(
                &rom->dma_info, dma_start, rom->file_size,
                entries, sizeof entries
            );

            if (result == ZELDA64_OK) {
                return result;
            }

            seek_pos += ZELDA64_DMA_ENTRY_SIZE;
        }

        offset += sizeof chunk;
    }
}

static enum zelda64_result
read_dmadata(struct zelda64_rom* rom) {
    // Navigate to the start of the DMADATA
    if (fseek(rom->file, rom->dma_info.offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }

    // Allocate a buffer large enough to hold our DMADATA
    rom->dma_table = calloc(rom->dma_info.count, sizeof *rom->dma_table);
    if (rom->dma_table == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    enum zelda64_result result = ZELDA64_OK;
    for (size_t i = 0; i < rom->dma_info.count; i += CHUNK_COUNT) {
        size_t const remaining = rom->dma_info.count - i;
        size_t const want = remaining < CHUNK_COUNT ? remaining : CHUNK_COUNT;

        uint8_t data[CHUNK_SIZE];
        if (fread(data, ZELDA64_DMA_ENTRY_SIZE, want, rom->file) != want) {
            result = ZELDA64_IO_ERROR;
            goto cleanup_table;
        }

        result = zelda64_read_dmadata(&rom->dma_table[i], want, data, CHUNK_SIZE);
        if (result != ZELDA64_OK) {
            goto cleanup_table;
        }
    }

    return ZELDA64_OK;

cleanup_table:
    free(rom->dma_table);
    rom->dma_table = NULL;
    return result;
}

static enum zelda64_result
read_rom_info(struct zelda64_rom* rom) {
    enum zelda64_result result = ZELDA64_OK;

    uint32_t makerom_offset = 0;
    uint32_t makerom_size = 0;
    result = zelda64_dma_entry_extent(&rom->dma_table[0], &makerom_offset, &makerom_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Allocate a temporary buffer for the MAKEROM
    uint8_t* makerom = malloc(makerom_size);
    if (makerom == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    if (fseek(rom->file, makerom_offset, SEEK_SET) != 0) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_makerom;
    }

    if (fread(makerom, makerom_size, 1, rom->file) != 1) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_makerom;
    }

    result = zelda64_read_rom_info(&rom->info, makerom, makerom_size);

cleanup_makerom:
    free(makerom);
    return result;
}

enum zelda64_result
zelda64_open_rom(char const* filename, struct zelda64_rom* rom) {
    enum zelda64_result result = ZELDA64_OK;
    if (filename == NULL || rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    *rom = (struct zelda64_rom){0};

    rom->filename = filename;
    rom->file = fopen(rom->filename, "rb");
    if (rom->file == NULL) {
        return ZELDA64_IO_ERROR;
    }

    if (fseek(rom->file, 0, SEEK_END) != 0) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_file;
    }

    long const file_size = ftell(rom->file);
    if (file_size < 0) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_file;
    }

    rom->file_size = (size_t) file_size;
    rewind(rom->file);

    // Get the DMADATA info from the ROM.
    result = get_dma_info(rom);
    if (result != ZELDA64_OK) {
        goto cleanup_file;
    }

    logf_info("Found DMADATA at offset 0x%08" PRIX32, rom->dma_info.offset);

    // Read the DMADATA to memory, we'll need it.
    result = read_dmadata(rom);
    if (result != ZELDA64_OK) {
        goto cleanup_file;
    }

    size_t counts[4] = {0};

    for (size_t i = 0; i < rom->dma_info.count; ++i) {
        struct zelda64_dma_entry entry = rom->dma_table[i];
        counts[zelda64_dma_entry_kind(&entry)]++;
    }

    logf_info("DMADATA has %zu entries: %zu uncompressed, %zu compressed, %zu empty, %zu deleted",
              rom->dma_info.count,
              counts[ZELDA64_DMA_UNCOMPRESSED],
              counts[ZELDA64_DMA_COMPRESSED],
              counts[ZELDA64_DMA_EMPTY],
              counts[ZELDA64_DMA_DELETED]);

    result = read_rom_info(rom);
    if (result != ZELDA64_OK) {
        goto cleanup_table;
    }

    logf_info("ROM is %.4s version %d (check code %016"PRIX64")",
              rom->info.header.game_code,
              rom->info.header.version + 1,
              rom->info.header.check_code);

    return ZELDA64_OK;

cleanup_table:
    free(rom->dma_table);
    rom->dma_table = NULL;
cleanup_file:
    fclose(rom->file);
    return result;
}

enum zelda64_result
zelda64_create_rom(char const* filename,
                   struct zelda64_dmadata_info const* dma_info,
                   struct zelda64_rom* rom) {
    if (filename == NULL || dma_info == NULL || rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    enum zelda64_result result = ZELDA64_OK;

    *rom = (struct zelda64_rom){0};

    rom->filename = filename;
    rom->file = fopen(rom->filename, "w+b");
    rom->file_size = 0;
    if (rom->file == NULL) {
        return ZELDA64_IO_ERROR;
    }

    // Allocate DMADATA for the ROM.
    rom->dma_info = *dma_info;
    rom->dma_table = calloc(rom->dma_info.count, sizeof *rom->dma_table);
    if (rom->dma_table == NULL) {
        result = ZELDA64_MEMORY_ERROR;
        goto cleanup_file;
    }

    return ZELDA64_OK;

cleanup_file:
    fclose(rom->file);
    rom->file = NULL;
    return result;
}

void
zelda64_close_rom(struct zelda64_rom* rom) {
    fclose(rom->file);
    free(rom->dma_table);
    *rom = (struct zelda64_rom){0};
}

enum zelda64_result
zelda64_copy_file(struct zelda64_rom* out_rom, uint32_t const offset,
                  struct zelda64_rom const* in_rom, size_t const file_index) {
    if (out_rom == NULL || in_rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    enum zelda64_result result = ZELDA64_OK;
    struct zelda64_dma_entry const in_entry = in_rom->dma_table[file_index];

    // We need to know the size of the file we're working with.
    uint32_t file_offset = 0;
    uint32_t file_size = 0;
    result = zelda64_dma_entry_extent(&in_entry, &file_offset, &file_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Set the read and write cursor of our ROMs.
    if (fseek(out_rom->file, offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }
    if (fseek(in_rom->file, file_offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }

    // Just a simple file copy loop from here on out.
    size_t bytes_out = 0;

    while (bytes_out < file_size) {
        uint8_t chunk[1024];
        size_t const remaining = file_size - bytes_out;
        size_t const have = remaining < sizeof chunk ? remaining : sizeof chunk;

        // Read input bytes.
        if (fread(chunk, have, 1, in_rom->file) != 1) {
            return ZELDA64_IO_ERROR;
        }

        // Write output bytes.
        if (fwrite(chunk, have, 1, out_rom->file) != 1) {
            return ZELDA64_IO_ERROR;
        }

        bytes_out += have;
    }

    // Update the DMA entry for the output ROM.
    out_rom->dma_table[file_index] = (struct zelda64_dma_entry){
        .vrom_start = in_entry.vrom_start,
        .vrom_end = in_entry.vrom_end,
        .rom_start = offset,
        .rom_end = 0x0,
    };

    // If the file got bigger, update our file size.
    if (out_rom->dma_table[file_index].vrom_end > out_rom->file_size) {
        out_rom->file_size = out_rom->dma_table[file_index].vrom_end;
    }

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_compress_file(struct zelda64_rom* out_rom, uint32_t const offset,
                      struct zelda64_rom const* in_rom, size_t const file_index) {
    if (out_rom == NULL || in_rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    enum zelda64_result result = ZELDA64_OK;
    struct zelda64_dma_entry const in_entry = in_rom->dma_table[file_index];

    // We need to know the size of the file we're working with.
    uint32_t file_offset = 0;
    uint32_t file_size = 0;
    result = zelda64_dma_entry_extent(&in_entry, &file_offset, &file_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Set the read and write cursor of our ROMs.
    if (fseek(out_rom->file, offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }
    if (fseek(in_rom->file, file_offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }

    // Initialize the decompressor.
    struct yaz0_stream stream = {0};
    if (yaz0_compress_init(&stream, YAZ0_DEFAULT_COMPRESSION, file_size)) {
        return ZELDA64_COMPRESSION_ERROR;
    }

    enum yaz0_result decomp_result = YAZ0_OK;
    enum yaz0_flush flush = YAZ0_NO_FLUSH;
    uint8_t in_chunk[1024];
    uint8_t out_chunk[1024];

    size_t bytes_read = 0;

    do {
        size_t const remaining = file_size - bytes_read;
        size_t const want = remaining < sizeof in_chunk ? remaining : sizeof in_chunk;

        stream.avail_in = fread(in_chunk, 1, want, in_rom->file);
        stream.next_in = in_chunk;
        bytes_read += stream.avail_in;

        flush = (bytes_read == file_size) ? YAZ0_FINISH : YAZ0_NO_FLUSH;

        // Run the decompressor in a loop until we've received all output data.
        do {
            stream.next_out = out_chunk;
            stream.avail_out = sizeof out_chunk;

            decomp_result = yaz0_compress(&stream, flush);
            if (decomp_result < YAZ0_OK) {
                yaz0_compress_end(&stream);
                return ZELDA64_COMPRESSION_ERROR;
            }

            // Write the decompressed bytes.
            size_t const have = sizeof out_chunk - stream.avail_out;
            if (fwrite(out_chunk, sizeof *out_chunk, have, out_rom->file) != have) {
                yaz0_compress_end(&stream);
                return ZELDA64_IO_ERROR;
            }
        } while (stream.avail_out == 0 && stream.avail_in != 0);
    } while (decomp_result != YAZ0_STREAM_END);

    // Ensure the file is padded:
    uint32_t const compressed_size = (stream.total_out + 15) & -16;

    yaz0_compress_end(&stream);

    // Update the DMA entry for the output ROM.
    out_rom->dma_table[file_index] = (struct zelda64_dma_entry){
        .vrom_start = in_entry.vrom_start,
        .vrom_end = in_entry.vrom_end,
        .rom_start = offset,
        .rom_end = offset + compressed_size,
    };

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_decompress_file(struct zelda64_rom* out_rom,
                        struct zelda64_rom const* in_rom, size_t const file_index) {
    if (out_rom == NULL || in_rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    enum zelda64_result result = ZELDA64_OK;
    struct zelda64_dma_entry const in_entry = in_rom->dma_table[file_index];

    // We need to know the size of the file we're working with.
    uint32_t file_offset = 0;
    uint32_t file_size = 0;
    result = zelda64_dma_entry_extent(&in_entry, &file_offset, &file_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Set the read and write cursor of our ROMs.
    if (fseek(out_rom->file, in_entry.vrom_start, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }
    if (fseek(in_rom->file, file_offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }

    // Initialize the decompressor.
    struct yaz0_stream stream = {0};
    if (yaz0_decompress_init(&stream)) {
        return ZELDA64_DECOMPRESSION_ERROR;
    }

    enum yaz0_result decomp_result = YAZ0_OK;
    enum yaz0_flush flush = YAZ0_NO_FLUSH;
    uint8_t in_chunk[1024];
    uint8_t out_chunk[1024];

    size_t bytes_read = 0;

    do {
        size_t const remaining = file_size - bytes_read;
        size_t const want = remaining < sizeof in_chunk ? remaining : sizeof in_chunk;

        stream.avail_in = fread(in_chunk, 1, want, in_rom->file);
        stream.next_in = in_chunk;
        bytes_read += stream.avail_in;

        flush = (bytes_read == file_size) ? YAZ0_FINISH : YAZ0_NO_FLUSH;

        // Run the decompressor in a loop until we've received all output data.
        do {
            stream.next_out = out_chunk;
            stream.avail_out = sizeof out_chunk;

            decomp_result = yaz0_decompress(&stream, flush);
            if (decomp_result < YAZ0_OK) {
                yaz0_decompress_end(&stream);
                return ZELDA64_DECOMPRESSION_ERROR;
            }

            // Write the decompressed bytes.
            size_t const have = sizeof out_chunk - stream.avail_out;
            if (fwrite(out_chunk, sizeof *out_chunk, have, out_rom->file) != have) {
                yaz0_decompress_end(&stream);
                return ZELDA64_IO_ERROR;
            }
        } while (stream.avail_out == 0 && stream.avail_in != 0);
    } while (decomp_result != YAZ0_STREAM_END);

    yaz0_decompress_end(&stream);

    // Update the DMA entry for the output ROM.
    out_rom->dma_table[file_index] = (struct zelda64_dma_entry){
        .vrom_start = in_entry.vrom_start,
        .vrom_end = in_entry.vrom_end,
        .rom_start = in_entry.vrom_start,
        .rom_end = 0x0,
    };

    // If the file got bigger, update our file size.
    if (out_rom->dma_table[file_index].vrom_end > out_rom->file_size) {
        out_rom->file_size = out_rom->dma_table[file_index].vrom_end;
    }

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_write_dmadata_to_rom(struct zelda64_rom* rom) {
    if (rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    // Navigate to the start of the DMADATA
    if (fseek(rom->file, rom->dma_info.offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }

    enum zelda64_result result = ZELDA64_OK;
    for (size_t i = 0; i < rom->dma_info.count; i += CHUNK_COUNT) {
        size_t const remaining = rom->dma_info.count - i;
        size_t const have = remaining < CHUNK_COUNT ? remaining : CHUNK_COUNT;

        uint8_t data[CHUNK_SIZE];
        result = zelda64_write_dmadata(data, CHUNK_SIZE, &rom->dma_table[i], have);
        if (result != ZELDA64_OK) {
            return result;
        }

        if (fwrite(data, ZELDA64_DMA_ENTRY_SIZE, have, rom->file) != have) {
            return ZELDA64_IO_ERROR;
        }
    }

    return ZELDA64_OK;
}

static enum zelda64_result
calculate_rom_check_code(struct zelda64_rom* rom) {
    // Refresh the ROM info, which is probably stale.
    enum zelda64_result result = read_rom_info(rom);
    if (result != ZELDA64_OK) {
        return result;
    }

    // We need the IPL3 section for this.
    // TODO: This smells of a library concern!
    uint32_t const ipl_offset = 0x40;
    uint32_t const ipl_size = 0xFC0;
    uint8_t* ipl = malloc(ipl_size);
    if (ipl == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    // Read the IPL3 into memory.
    if (fseek(rom->file, ipl_offset, SEEK_SET) != 0) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_ipl;
    }

    if (fread(ipl, ipl_size, 1, rom->file) != 1) {
        result = ZELDA64_IO_ERROR;
        goto cleanup_ipl;
    }


    struct zelda64_check_code_state state;
    result = zelda64_cic_check_code_init(rom->info.cic, ipl, ipl_size, &state);
    if (result != ZELDA64_OK) {
        goto cleanup_ipl;
    }

    // TODO: Where does 0x101000 come from? The library of course.
    while (state.offset < 0x101000) {
        if (fseek(rom->file, (long) state.offset, SEEK_SET) != 0) {
            result = ZELDA64_IO_ERROR;
            goto cleanup_ipl;
        }

        uint8_t chunk[1024];
        size_t const remaining = 0x101000 - state.offset;
        size_t const want = remaining < sizeof chunk ? remaining : sizeof chunk;

        if (fread(chunk, sizeof *chunk, want, rom->file) != want) {
            result = ZELDA64_IO_ERROR;
            goto cleanup_ipl;
        }

        result = zelda64_cic_check_code(&state, chunk, want);
        if (result != ZELDA64_OK) {
            goto cleanup_ipl;
        }
    }

    uint64_t const check_code = zelda64_cic_check_code_end(&state);
    rom->info.header.check_code = check_code;
    result = ZELDA64_OK;

cleanup_ipl:
    free(ipl);
    return result;
}

enum zelda64_result zelda64_finalize_rom(struct zelda64_rom* rom) {
    enum zelda64_result result = calculate_rom_check_code(rom);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Find the MAKEROM (it's always at offset 0, but just in case...)
    uint32_t makerom_offset = 0;
    uint32_t makerom_size = 0;
    result = zelda64_dma_entry_extent(&rom->dma_table[0], &makerom_offset, &makerom_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Write the new header out.
    uint8_t header[ZELDA64_ROM_HEADER_SIZE];
    result = zelda64_write_rom_header(header, ZELDA64_ROM_HEADER_SIZE, &rom->info.header);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Write the new header to the MAKEROM.
    if (fseek(rom->file, makerom_offset, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }
    if (fwrite(header, ZELDA64_ROM_HEADER_SIZE, 1, rom->file) != 1) {
        return ZELDA64_IO_ERROR;
    }

    return ZELDA64_OK;
}

static size_t
round_up_pow2(size_t value) {
    if (value == 0) {
        return 1;
    }

    value -= 1;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return value + 1;
}

static enum zelda64_result
zelda64_fill(struct zelda64_rom* rom, size_t const start, size_t const end,
             enum zelda64_fill_mode const mode) {
    uint8_t ramp[256];
    for (size_t i = 0; i < sizeof ramp; ++i) {
        switch (mode) {
            case ZELDA64_FILL_ZERO:
                ramp[i] = 0;
                break;

            case ZELDA64_FILL_RAMP:
                ramp[i] = (uint8_t) i;
                break;

            default:
                abort();
        }
    }

    if (fseek(rom->file, (long) start, SEEK_SET) != 0) {
        return ZELDA64_IO_ERROR;
    }

    for (size_t offset = start; offset < end;) {
        size_t const phase = offset & 0xFF;
        size_t const room = sizeof ramp - phase;
        size_t const remaining = end - offset;
        size_t const want = remaining < room ? remaining : room;

        if (fwrite(&ramp[phase], 1, want, rom->file) != want) {
            return ZELDA64_IO_ERROR;
        }

        offset += (uint32_t) want;
    }

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_pad_rom(struct zelda64_rom* rom, enum zelda64_fill_mode const fill) {
    size_t const old_size = rom->file_size;
    size_t const padded_size = round_up_pow2(rom->file_size);
    return zelda64_fill(rom, old_size, padded_size, fill);
}
