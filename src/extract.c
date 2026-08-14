/*
 * extract.c: Nintendo 64 Zelda ROM file extractor
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zelda64/zelda64.h>

#define BINARY_FILE_EXT ".bin"
#define YAZ0_FILE_EXT ".szs"

static enum zelda64_result
write_file(char const* filename,
           uint8_t const* buffer, size_t const size,
           struct zelda64_error* error) {
    // We have the whole file in memory now, so we can write it out.
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        error->result = ZELDA64_ERRNO;
        error->sys_error = errno;
        return error->result;
    }

    // And perform the copy.
    if (buffer != NULL && size > 0) {
        if (fwrite(buffer, sizeof(uint8_t), size, file) != size) {
            error->sys_error = errno;
            error->result = ZELDA64_ERRNO;
            fclose(file);
            return error->result;
        }
    }

    fclose(file);
    return ZELDA64_OK;
}

static enum zelda64_result
extract_binary_file(char const* directory, uint32_t const file_size,
                    struct zelda64_rom const* rom, size_t const index,
                    struct zelda64_error* error) {
    // Check if the filename can fit in the buffer.
    char filename[256] = {0};
    size_t const filename_len = snprintf(NULL, 0, "%s/%04zX%s", directory, index, BINARY_FILE_EXT);
    if (filename_len >= sizeof filename) {
        fprintf(stderr, "extract: error: destination path too long\n");
        return ZELDA64_OUT_OF_RANGE;
    }

    // Format the output filename and write the file.
    snprintf(filename, sizeof filename, "%s/%04zX%s", directory, index, BINARY_FILE_EXT);

    if (file_size > 0) {
        uint8_t* buffer = malloc(file_size);
        if (buffer == NULL) {
            error->result = ZELDA64_MEMORY_ERROR;
            return error->result;
        }

        if (zelda64_read_file(buffer, file_size, rom, index, error) != file_size) {
            fprintf(stderr, "extract: error: cannot read file %zu: %s\n",
                    index, zelda64_error_string(error));

            free(buffer);
            return error->result;
        }

        if (write_file(filename, buffer, file_size, error) != ZELDA64_OK) {
            free(buffer);
            return error->result;
        }

        free(buffer);
    } else {
        if (write_file(filename, NULL, 0, error) != ZELDA64_OK) {
            return error->result;
        }
    }

    return ZELDA64_OK;
}

static enum zelda64_result
extract_compressed_file(char const* directory, uint32_t const file_size,
                        struct zelda64_rom const* rom, size_t const index,
                        struct zelda64_error* error) {
    // Check if the filename can fit in the buffer.
    char filename[256] = {0};
    size_t const filename_len = snprintf(NULL, 0, "%s/%04zX%s", directory, index, YAZ0_FILE_EXT);
    if (filename_len >= sizeof filename) {
        fprintf(stderr, "extract: error: destination path too long\n");
        return ZELDA64_OUT_OF_RANGE;
    }

    // Format the output filename and write the file.
    snprintf(filename, sizeof filename, "%s/%04zX%s", directory, index, YAZ0_FILE_EXT);

    uint8_t* buffer = malloc(file_size);
    if (buffer == NULL) {
        error->result = ZELDA64_MEMORY_ERROR;
        return error->result;
    }

    if (zelda64_read_storage(buffer, file_size, rom, index, 0, error) != file_size) {
        fprintf(stderr, "extract: error: cannot read file %zu: %s\n",
                index, zelda64_error_string(error));

        free(buffer);
        return error->result;
    }

    if (write_file(filename, buffer, file_size, error) != ZELDA64_OK) {
        free(buffer);
        return error->result;
    }

    free(buffer);
    return ZELDA64_OK;
}

static enum zelda64_result
extract_file(char const* directory,
             struct zelda64_rom const* rom, size_t const index,
             struct zelda64_error* error) {
    // We need to query some information about the file first.
    struct zelda64_stat st;
    if (zelda64_stat(&st, rom, index, error) != ZELDA64_OK) {
        // If the file is deleted, skip it.
        if (error->result == ZELDA64_DELETED) {
            error->result = ZELDA64_OK;
            return error->result;
        }

        // We cannot query this file, and it's not deleted.
        // Something bad happened, so we're bailing.
        fprintf(stderr,
                "extract: error: cannot get information about file %zu: %s\n",
                index, zelda64_error_string(error));

        return error->result;
    }

    switch (st.method) {
        case ZELDA64_METHOD_STORE: {
            if (extract_binary_file(directory, st.file_size, rom, index, error) < 0) {
                return error->result;
            }
            break;
        }

        case ZELDA64_METHOD_YAZ0: {
            if (extract_compressed_file(directory, st.size, rom, index, error) < 0) {
                return error->result;
            }
            if (extract_binary_file(directory, st.file_size, rom, index, error) < 0) {
                return error->result;
            }
            break;
        }
    }

    return ZELDA64_OK;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: extract rom path\n");
        return EXIT_FAILURE;
    }

    struct zelda64_error error = {0};
    struct zelda64_rom* rom = zelda64_open(argv[1], &error);
    if (rom == NULL) {
        fprintf(stderr, "extract: error: cannot open ROM: %s", zelda64_error_string(&error));
        return EXIT_FAILURE;
    }

    size_t const count = zelda64_file_count(rom, &error);
    for (zelda64_index_t i = 0; i < count; ++i) {
        if (extract_file(argv[2], rom, i, &error) != ZELDA64_OK) {
            zelda64_close(rom);
            return EXIT_FAILURE;
        }
    }

    zelda64_close(rom);
    return EXIT_SUCCESS;
}
