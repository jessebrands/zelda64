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

static void
error_with_code(struct zelda64_error const* error, char const* what, char const* slug) {
    fprintf(stderr, "error: %s: %s\n", what, zelda64_error_string(error));
    fprintf(stderr, "exit %d (%s)\n", EXIT_FAILURE, slug);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        return EXIT_FAILURE;
    }

    char const* in_filename = argv[1];

    struct zelda64_error error;
    struct zelda64_rom* rom = zelda64_open(in_filename, &error);
    if (rom == NULL) {
        error_with_code(&error, "could not open ROM", "open-failed");
    }

    printf("decompress: DMADATA has %zu entries\n", zelda64_dmadata_entries_count(rom));

    zelda64_close(rom);
    return EXIT_SUCCESS;
}
