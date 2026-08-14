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

enum zelda64_result {
    ZELDA64_OK = 0,
    ZELDA64_INVALID_PARAMETER = -1,
    ZELDA64_MEMORY_ERROR = -2,
    ZELDA64_OUT_OF_RANGE = -3,
    ZELDA64_TRUNCATED = -4,
    ZELDA64_ERRNO = -5,
    ZELDA64_NO_DMADATA = -6,
    ZELDA64_DELETED = -7,
};

enum zelda64_entry_kind {
    ZELDA64_ENTRY_EMPTY = 0,
    ZELDA64_ENTRY_DELETED,
    ZELDA64_ENTRY_STORED,
    ZELDA64_ENTRY_COMPRESSED,
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
    uint32_t vrom_start;
    uint32_t vrom_end;
    uint32_t offset;
    uint32_t size;
    enum zelda64_entry_kind kind;
};

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
 * Returns information about a file in a ROM.
 */
ZELDA64_API enum zelda64_result
zelda64_stat(struct zelda64_stat* st,
             struct zelda64_rom const* rom, size_t index,
             struct zelda64_error* error);


/*
 * Returns how many files are in a ROM.
 */
ZELDA64_API size_t
zelda64_file_count(struct zelda64_rom const* rom);

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
