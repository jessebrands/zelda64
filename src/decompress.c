/*
 * decompress.h: Nintendo 64 Zelda ROM decompressor
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

#include <stdlib.h>
#include <time.h>

#include "decompress.h"
#include "log.h"

int
run_decompress(struct zelda64_decompress_spec const* spec,
               struct zelda64_rom* rom, char const* out_filename) {
    struct zelda64_error error = {0};

    struct zelda64_dmadata_layout* layout = zelda64_decompress(rom, &error);
    if (layout == NULL) {
        logf_error("cannot build layout: %s", zelda64_error_string(&error));
        return EXIT_FAILURE;
    }

    struct zelda64_write_options const write_options = {
        .pack = spec->pack,
        .pad = spec->pad,
    };

    log_info("Decompressing ROM");
    clock_t const start = clock();
    zelda64_write(out_filename, layout, &write_options, &error);
    zelda64_free_layout(layout);

    if (error.result != ZELDA64_OK) {
        logf_error("cannot write %s: %s", out_filename, zelda64_error_string(&error));
        return EXIT_FAILURE;
    }

    clock_t const end = clock();
    double const elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    logf_info("Decompression finished in %.2f seconds", elapsed);
    return EXIT_SUCCESS;
}
