/*
 * zelda64.c: program entrypoint
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <zelda64/zelda64.h>

#include "decompress.h"
#include "log.h"
#include "options.h"

#define EXIT_USAGE 2

static void
print_version(FILE* stream) {
    fprintf(stream, "zelda64 %s\n", zelda64_version_string());
    fprintf(stream, "Copyright (C) 2026  Jesse Gerard Brands\n");
    fprintf(stream, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stream, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
    fprintf(stream, "\n");
}

static void
print_usage_string(FILE* stream) {
    fprintf(stream, "Usage: zelda64 [options]... [-it] [-dpzx] rom [out]\n");
}

static void
print_usage(FILE* stream) {
    print_usage_string(stream);
    fprintf(stream, "\n");
}

static void
print_help(FILE* stream) {
    print_usage_string(stream);
    fprintf(stream, "Manipulate Nintendo 64 Zelda ROMs\n");
    fprintf(stream, "\n");
    fprintf(stream, "Operations:\n");
    fprintf(stream, "  -d, --decompress        decompress rom\n");
    fprintf(stream, "  -p, --patch=PATCH       patch rom\n");
    fprintf(stream, "  -z, --compress          compress rom\n");
    fprintf(stream, "  -x, --extract           extract rom files\n");
    fprintf(stream, "\n");
    fprintf(stream, "Commands:\n");
    fprintf(stream, "  -i, --info              print rom info\n");
    fprintf(stream, "  -t, --dmadata           print dmadata table\n");
    fprintf(stream, "\n");
    fprintf(stream, "Options:\n");
    fprintf(stream, "  -l, --level=N           compression level\n");
    fprintf(stream, "  -L, --copy-list=FILE    specify dmadata entries to copy\n");
    fprintf(stream, "      --pad=PAD           pad output to power of 2\n");
    fprintf(stream, "  -P, --pack=PACK         pack output files\n");
    fprintf(stream, "  -v, --verbose           verbose output (repeat to increase)\n");
    fprintf(stream, "      --version           print version information\n");
    fprintf(stream, "      --help              print this message\n");
    fprintf(stream, "\n");
}

int main(int argc, char** argv) {
    struct zelda64_options options = {0};
    enum zelda64_parse_result const parse_result = parse_options(&options, argc, argv);
    if (parse_result == ZELDA64_PARSE_USAGE) {
        print_usage(stderr);
        return EXIT_USAGE;
    }
    if (parse_result == ZELDA64_PARSE_VERSION) {
        print_version(stdout);
        return EXIT_SUCCESS;
    }
    if (parse_result == ZELDA64_PARSE_HELP) {
        print_help(stdout);
        return EXIT_SUCCESS;
    }

    if (options.in_filename == NULL) {
        log_error("no input rom");
    }

    int status = EXIT_SUCCESS;
    struct zelda64_error error = {0};

    // Open the input ROM.
    struct zelda64_rom* rom = zelda64_open(options.in_filename, &error);
    if (rom == NULL) {
        logf_error("cannot open %s: %s", options.in_filename, zelda64_error_string(&error));
        return EXIT_FAILURE;
    }

    // Present a greeting.
    char compressor_name[128] = {0};
    zelda64_compressor_name(compressor_name, sizeof compressor_name);
    logf_info("zelda64 %s using %s", zelda64_version_string(), compressor_name);
    log_info("Copyright (C) 2026 Jesse Gerard Brands");

    // Loop over the stages
    clock_t const start = clock();
    for (size_t i = 0; i < options.stage_count; ++i) {
        char const* out = (i + 1 == options.stage_count) ? options.out_filename : NULL;

        switch (options.stages[i].stage) {
            case ZELDA64_STAGE_DECOMPRESS:
                status = run_decompress(&options.stages[i].as.decompress, rom, out);
                break;
        }
        if (status != EXIT_SUCCESS) { break; }
    }

    clock_t const end = clock();
    if (options.stage_count > 1) {
        double const elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        logf_info("Finished in %.2f seconds", elapsed);
    }

    zelda64_close(rom);
    return status;
}
