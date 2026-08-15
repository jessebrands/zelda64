/*
 * decompress.c: Nintendo 64 Zelda ROM decompressor
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
#include <stdlib.h>

#include <zelda64/zelda64.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        return EXIT_FAILURE;
    }

    char const* in_filename = argv[1];

    struct zelda64_error error;
    struct zelda64_rom* rom = zelda64_open(in_filename, &error);
    if (rom == NULL) {
        fprintf(stderr,
                "decompress: error: failed to open ROM: %s\n",
                zelda64_error_string(&error));
        return EXIT_FAILURE;
    }

    // Create a decompressed layout.
    struct zelda64_dmadata_layout* layout = zelda64_decompress(rom, &error);
    if (layout == NULL) {
        fprintf(stderr,
                "decompress: error: failed to create decompressed layout from ROM: %s\n",
                zelda64_error_string(&error));
        zelda64_close(rom);
        return EXIT_FAILURE;
    }

    // This configuration creates a decompressed ROM compatible with the
    // Ocarina of Time Randomizer. It hardcodes ROM offsets so we can't really
    // go ahead and pack it more densely.
    struct zelda64_write_options const options = {
        .pack = ZELDA64_PACK_SPARSE,
        .pad = ZELDA64_PAD_ZERO,
    };

    // Write the output ROM.
    if (zelda64_write(argv[2], layout, &options, &error) != ZELDA64_OK) {
        fprintf(stderr, "decompress: error: failed to write output ROM: %s\n",
                zelda64_error_string(&error));
        zelda64_free_layout(layout);
        zelda64_close(rom);
        return EXIT_FAILURE;
    }

    zelda64_free_layout(layout);
    zelda64_close(rom);
    return EXIT_SUCCESS;
}
