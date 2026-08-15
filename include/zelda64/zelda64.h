/*
 * zelda64.h: Nintendo 64 Zelda ROM library
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

#include <zelda64/config.h>

#include <stddef.h>
#include <stdint.h>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t zelda64_index_t;
typedef size_t zelda64_offset_t;
typedef int64_t zelda64_ssize_t;

enum zelda64_result {
    ZELDA64_OK = 0,
    ZELDA64_INVALID_PARAMETER = -1,
    ZELDA64_MEMORY_ERROR = -2,
    ZELDA64_OUT_OF_RANGE = -3,
    ZELDA64_TRUNCATED = -4,
    ZELDA64_ERRNO = -5,
    ZELDA64_NO_DMADATA = -6,
    ZELDA64_DELETED = -7,
    ZELDA64_DECOMPRESS_ERROR = -8,
    ZELDA64_COMPRESS_ERROR = -9,
    ZELDA64_INVALID_ROM = -10,
    ZELDA64_UNSUPPORTED_CIC = -11,
};

enum zelda64_cic {
    ZELDA64_CIC_UNKNOWN = 0,
    ZELDA64_CIC_6105 = 6105,
};

enum zelda64_method {
    ZELDA64_METHOD_STORE = 0,
    ZELDA64_METHOD_YAZ0 = 1,
};

enum zelda64_operation {
    ZELDA64_OP_COPY = 0,
    ZELDA64_OP_COMPRESS = 1,
    ZELDA64_OP_DECOMPRESS = 2,
    ZELDA64_OP_DELETE = 3,
};

enum zelda64_pack {
    ZELDA64_PACK_SPARSE = 0,
    ZELDA64_PACK_DENSE = 1,
};

enum zelda64_pad {
    ZELDA64_PAD_NONE = 0,
    ZELDA64_PAD_ZERO = 1,
    ZELDA64_PAD_RAMP = 2,
};

typedef void* (zelda64_alloc_func)(void* opaque, size_t size);

typedef void (zelda64_free_func)(void* opaque, void* ptr);

struct zelda64_allocator {
    zelda64_alloc_func* alloc;
    zelda64_free_func* free;
    void* opaque;
};

struct zelda64_error {
    enum zelda64_result result;
    int sys_error;
};

struct zelda64_rom;

struct zelda64_dmadata {
    uint32_t vrom_start;
    uint32_t vrom_end;
    uint32_t rom_start;
    uint32_t rom_end;
};

struct zelda64_stat {
    enum zelda64_method method; // What is stored in the ROM
    uint32_t offset; // Offset in the ROM
    uint32_t size; // Size of the file on the ROM
    uint32_t file_size; // Real size of the file
};

struct zelda64_write_options {
    enum zelda64_pack pack;
    enum zelda64_pad pad;
};

/*
 * Describes the layout of a Nintendo 64 Zelda ROM.
 */
struct zelda64_dmadata_layout;

/*
 * Returns the default memory allocator.
 */
ZELDA64_API struct zelda64_allocator
zelda64_default_allocator(void);

/*
 * Opens a Nintendo 64 Zelda ROM.
 */
ZELDA64_API struct zelda64_rom*
zelda64_open(char const* filename, struct zelda64_error* error);

/*
 * Opens a Nintendo 64 Zelda ROM. An allocator must be supplied by the caller
 * which the library will use for memory allocations.
 */
ZELDA64_API struct zelda64_rom*
zelda64_open_with_allocator(char const* filename, struct zelda64_allocator allocator, struct zelda64_error* error);

/*
 * Closes a Nintendo 64 Zelda ROM and releases all associated resources.
 */
ZELDA64_API void
zelda64_close(struct zelda64_rom* rom);

/*
 * Returns how many files are in a ROM.
 */
ZELDA64_API size_t
zelda64_file_count(struct zelda64_rom const* rom, struct zelda64_error* error);

/*
 * Returns information about a file in a ROM.
 */
ZELDA64_API enum zelda64_result
zelda64_stat(struct zelda64_stat* st,
             struct zelda64_rom const* rom, zelda64_index_t index,
             struct zelda64_error* error);

/*
 * Returns a read-only pointer to the DMADATA.
 */
ZELDA64_API struct zelda64_dmadata const*
zelda64_dmadata(struct zelda64_rom const* rom,
                struct zelda64_error* error);

/*
 * Returns a read-only pointer to a DMADATA entry.
 */
ZELDA64_API struct zelda64_dmadata const*
zelda64_dmadata_entry(struct zelda64_rom const* rom, zelda64_index_t index,
                      struct zelda64_error* error);

/*
 * Reads bytes directly from the ROM to a buffer.
 */
ZELDA64_API zelda64_ssize_t
zelda64_read_storage(void* buffer, size_t size,
                     struct zelda64_rom const* rom,
                     zelda64_index_t index, uint32_t offset,
                     struct zelda64_error* error);

/*
 * Reads a file from the ROM into a buffer.
 */
ZELDA64_API zelda64_ssize_t
zelda64_read_file(void* buffer, size_t size,
                  struct zelda64_rom const* rom, zelda64_index_t index,
                  struct zelda64_error* error);

/*
 * Returns the size of the ROM in bytes.
 */
ZELDA64_API zelda64_ssize_t
zelda64_rom_size(struct zelda64_rom const* rom, struct zelda64_error* error);

/*
 * Creates a decompressed layout from a ROM. Uses ROM's allocator.
 */
ZELDA64_API struct zelda64_dmadata_layout*
zelda64_decompress(struct zelda64_rom const* rom, struct zelda64_error* error);

/*
 * Creates a decompressed layout from a ROM.
 */
ZELDA64_API struct zelda64_dmadata_layout*
zelda64_decompress_with_allocator(struct zelda64_rom const* rom,
                                  struct zelda64_allocator allocator,
                                  struct zelda64_error* error);

ZELDA64_API void
zelda64_free_layout(struct zelda64_dmadata_layout* layout);

ZELDA64_API enum zelda64_result
zelda64_write(char const* filename,
              struct zelda64_dmadata_layout const* layout,
              struct zelda64_write_options const* options,
              struct zelda64_error* error);

/*
 * Returns a human-readable string for an error.
 */
ZELDA64_API char const*
zelda64_error_string(struct zelda64_error const* error);

/*
 * Returns a packed integer representing the library version.
 */
ZELDA64_API int
zelda64_version(void);

/*
 * Returns the library version as a string.
 */
ZELDA64_API char const*
zelda64_version_string(void);

#ifdef __cplusplus
}
#endif

#endif //LIBZELDA64_ZELDA64_H
