/*
 * compress.c: Nintendo 64 Zelda ROM compressor
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

#define DMADATA_OVERRIDE_FILENAME "dmaTable.dat"

static enum zelda64_result
read_dma_overrides(struct zelda64_dmadata_layout* layout,
                   struct zelda64_error* error) {
    FILE* file = fopen(DMADATA_OVERRIDE_FILENAME, "r");
    if (file == NULL) {
        error->result = ZELDA64_ERRNO;
        error->sys_error = errno;
        return ZELDA64_ERRNO;
    }

    int32_t entry;

    while (fscanf(file, "%d", &entry) == 1) {
        if (entry < 0) {
            zelda64_layout_set_operation(layout, ~entry + 1, ZELDA64_OP_DELETE, error);
        } else {
            zelda64_layout_set_operation(layout, entry, ZELDA64_OP_COPY, error);
        }

        if (error->result != ZELDA64_OK) {
            fclose(file);
            return error->result;
        }
    }

    fclose(file);
    return ZELDA64_OK;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        return EXIT_FAILURE;
    }

    char const* in_filename = argv[1];

    struct zelda64_error error = {0};
    struct zelda64_rom* rom = zelda64_open(in_filename, &error);
    if (rom == NULL) {
        fprintf(stderr,
                "compress: error: failed to open ROM: %s\n",
                zelda64_error_string(&error));
        return EXIT_FAILURE;
    }

    // Create a decompressed layout.
    struct zelda64_dmadata_layout* layout = zelda64_compress(rom, &error);
    if (layout == NULL) {
        fprintf(stderr,
                "compress: error: failed to create compressed layout from ROM: %s\n",
                zelda64_error_string(&error));
        zelda64_close(rom);
        return EXIT_FAILURE;
    }

    if (read_dma_overrides(layout, &error) != ZELDA64_OK) {
        fprintf(stderr, "compress: error: failed to read %s: %s\n",
                DMADATA_OVERRIDE_FILENAME,
                zelda64_error_string(&error));
        zelda64_free_layout(layout);
        zelda64_close(rom);
        return EXIT_FAILURE;
    }

    // Creates a Nintendo-like ROM.
    struct zelda64_write_options const options = {
        .pack = ZELDA64_PACK_DENSE,
        .pad = ZELDA64_PAD_RAMP,
    };

    // Write the output ROM.
    if (zelda64_write(argv[2], layout, &options, &error) != ZELDA64_OK) {
        fprintf(stderr, "compress: error: failed to write output ROM: %s\n",
                zelda64_error_string(&error));
        zelda64_free_layout(layout);
        zelda64_close(rom);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
