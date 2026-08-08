/*
 * compress.c: Nintendo 64 Zelda ROM Compressor
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

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zelda64/zelda64.h>

#include "log.h"
#include "rom.h"

#define COMPRESS_PROGRAM_NAME "compress"

enum option_result {
    OPTIONS_OK,
    OPTIONS_DONE,
    OPTIONS_USAGE,
};

struct compress_options {
    char const* in_filename;
    char const* out_filename;
    char const* cache_filename;
    char const* copy_list_filename;
    int compression_level;
};

static void
print_usage(FILE* stream) {
    fprintf(stream, "Usage: decompress [--cache FILE] [--copy-list FILE] rom outfile\n");
    fprintf(stream, "Compress a Nintendo 64 Zelda ROM.\n");
    fprintf(stream, "\n");
    fprintf(stream, "Options:\n");
    fprintf(stream, "  -l, --level N           compression level (default: 9)\n");
    fprintf(stream, "  -C, --cache FILE        write a new or update an existing compression cache\n");
    fprintf(stream, "  -L, --copy-list FILE    create a file with a list of indices that\n");
    fprintf(stream, "                            were copied instead of decompressed\n");
    fprintf(stream, "  -v, --verbose           print logging messages (repeat to increase\n");
    fprintf(stream, "                            verbosity)\n");
    fprintf(stream, "      --version           print version information\n");
    fprintf(stream, "      --help              print this message\n");
    fprintf(stream, "\n");
}

static void
print_version(FILE* stream) {
    fprintf(stream, "compress (libzelda64-utils) %s\n", zelda64_version_string());
    fprintf(stream, "Copyright (C) 2026  Jesse Gerard Brands\n");
    fprintf(stream, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stream, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
    fprintf(stream, "\n");
}

static enum option_result
usage_error(void) {
    fprintf(stderr, "Try '" COMPRESS_PROGRAM_NAME " --help' for more information.\n");
    return OPTIONS_USAGE;
}

static int
result_error(enum zelda64_result const result, char const* what) {
    char const* reason = "";
    if (result == ZELDA64_IO_ERROR) {
        reason = strerror(errno);
    } else {
        reason = zelda64_result_string(result);
    }

    logf_error("%s: %s", what, reason);
    return EXIT_FAILURE;
}

static bool
is_long_option(char const* option, size_t const length, char const* name) {
    return strlen(name) == length && memcmp(option, name, length) == 0;
}

static char const*
take_short_argument(char const* rest, int const argc, char** argv, int* index) {
    if (*rest != '\0') {
        return rest;
    }
    if (*index + 1 >= argc) {
        return NULL;
    }
    return argv[++(*index)];
}

static char const*
take_long_argument(char const* value, int const argc, char** argv, int* index) {
    if (value != NULL) {
        return value;
    }
    if (*index + 1 >= argc) {
        return NULL;
    }
    return argv[++(*index)];
}

static enum option_result
parse_options(struct compress_options* options, int const argc, char** argv) {
    *options = (struct compress_options){0};

    bool stop_parsing = false;
    int arguments = 0;

    for (int i = 1; i < argc; ++i) {
        char const* const arg = argv[i];

        if (stop_parsing || arg[0] != '-' || arg[1] == '\0') {
            switch (arguments++) {
                case 0:
                    options->in_filename = arg;
                    break;

                case 1:
                    options->out_filename = arg;
                    break;

                default:
                    log_error("too many arguments");
                    return usage_error();
            }
            continue;
        }

        if (arg[1] == '-') {
            char const* const option = &arg[2];

            /* "--" on its own ends option parsing. */
            if (*option == '\0') {
                stop_parsing = true;
                continue;
            }

            char const* const equals = strchr(option, '=');
            size_t const length = equals != NULL ? (size_t) (equals - option) : strlen(option);
            char const* const value = equals != NULL ? equals + 1 : NULL;

            if (is_long_option(option, length, "help")) {
                print_usage(stdout);
                return OPTIONS_DONE;
            }

            if (is_long_option(option, length, "version")) {
                print_version(stdout);
                return OPTIONS_DONE;
            }

            if (is_long_option(option, length, "verbose")) {
                if (value != NULL) {
                    log_error("option '--verbose' takes no argument");
                    return usage_error();
                }
                log_verbosity += 1;
                continue;
            }

            if (is_long_option(option, length, "cache")) {
                options->cache_filename = take_long_argument(value, argc, argv, &i);
                if (options->cache_filename == NULL) {
                    log_error("option '--cache' requires an argument");
                    return usage_error();
                }
                continue;
            }

            if (is_long_option(option, length, "copy-list")) {
                options->copy_list_filename = take_long_argument(value, argc, argv, &i);
                if (options->copy_list_filename == NULL) {
                    log_error("option '--copy-list' requires an argument");
                    return usage_error();
                }
                continue;
            }

            logf_error("unrecognized option '%s'", arg);
            return usage_error();
        }

        for (char const* p = &arg[1]; *p != '\0'; ++p) {
            switch (*p) {
                case 'v':
                    log_verbosity += 1;
                    break;

                case 'C':
                case 'L': {
                    char const letter = *p;
                    char const* const value = take_short_argument(p + 1, argc, argv, &i);
                    if (value == NULL) {
                        logf_error("option '-%c' requires an argument", letter);
                        return usage_error();
                    }
                    if (letter == 'C') {
                        options->cache_filename = value;
                    } else {
                        options->copy_list_filename = value;
                    }

                    // Yes, goto, because we need to stop scanning here.
                    goto next_argument;
                }

                default:
                    logf_error("unrecognized option '-%c'", *p);
                    return usage_error();
            }
        }

    next_argument:
        /* intentionally blank */;
    }

    if (options->in_filename == NULL) {
        log_error("no input ROM given");
        return usage_error();
    }

    if (options->out_filename == NULL) {
        log_error("no output file given");
        return usage_error();
    }

    return OPTIONS_OK;
}

int main(int argc, char** argv) {
    log_program = COMPRESS_PROGRAM_NAME;
    int exit_code = EXIT_SUCCESS;

    struct compress_options options;
    switch (parse_options(&options, argc, argv)) {
        case OPTIONS_DONE:
            return EXIT_SUCCESS;

        case OPTIONS_USAGE:
            return EXIT_FAILURE;

        case OPTIONS_OK:
            break;
    }

    // Let's say hello :-)
    logf_info("compress (libzelda64-utils) %s", zelda64_version_string());
    log_info("Copyright (C) 2026  Jesse Gerard Brands");

    struct zelda64_rom in_rom;
    enum zelda64_result result = zelda64_open_rom(options.in_filename, &in_rom);
    if (result != ZELDA64_OK) {
        return result_error(result, "could not open input ROM");
    }

    struct zelda64_rom out_rom;
    result = zelda64_create_rom(options.out_filename, COMPRESSED_SIZE, &in_rom.dma_info, &out_rom);
    if (result != ZELDA64_OK) {
        exit_code = result_error(result, "could not open output ROM");
        goto cleanup_in_rom;
    }

    // This should not be hard coded, but for now...
    int* ops = calloc(out_rom.dma_info.count, sizeof(*ops));
    if (ops == NULL) {
        exit_code = result_error(ZELDA64_MEMORY_ERROR, "could not allocate copy list");
        goto cleanup_in_rom;
    }

    // TODO: This should be threaded in by the decompressor.
    ops[0] = 1;
    ops[1] = 1;
    ops[2] = 1;
    ops[3] = 1;
    ops[4] = 1;
    ops[5] = 1;
    ops[6] = 1;
    ops[7] = 1;
    ops[8] = 1;
    ops[9] = 1;

    ops[15] = 1;
    ops[16] = 1;
    ops[17] = 1;
    ops[18] = 1;
    ops[19] = 1;
    ops[20] = 1;
    ops[21] = 1;
    ops[22] = 1;
    ops[23] = 1;
    ops[24] = 1;
    ops[25] = 1;
    ops[26] = 1;

    ops[942] = 1;
    ops[944] = 1;
    ops[946] = 1;
    ops[948] = 1;
    ops[950] = 1;
    ops[952] = 1;
    ops[954] = 1;
    ops[956] = 1;
    ops[958] = 1;
    ops[960] = 1;
    ops[962] = 1;
    ops[964] = 1;
    ops[966] = 1;
    ops[968] = 1;
    ops[970] = 1;
    ops[972] = 1;
    ops[974] = 1;
    ops[976] = 1;
    ops[978] = 1;
    ops[980] = 1;
    ops[982] = 1;
    ops[984] = 1;
    ops[986] = 1;
    ops[988] = 1;
    ops[990] = 1;
    ops[992] = 1;
    ops[994] = 1;
    ops[996] = 1;
    ops[998] = 1;
    ops[1000] = 1;
    ops[1002] = 1;
    ops[1004] = 1;

    log_info("Compressing ROM");
    clock_t const start = clock();

    uint32_t rom_offset = 0;
    for (size_t i = 0; i < in_rom.dma_info.count; ++i) {
        struct zelda64_dma_entry const in_entry = in_rom.dma_table[i];
        enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(&in_entry);

        if (ops[i] != 0) {
            logf_progress("[%4zu/%4zu] Copying file", i + 1, in_rom.dma_info.count);

            result = zelda64_copy_file(&out_rom, rom_offset, &in_rom, i);
            if (result != ZELDA64_OK) {
                exit_code = result_error(result, "failed to copy file");
                goto cleanup_out_rom;
            }

            rom_offset += in_entry.vrom_end - in_entry.vrom_start;
            continue;
        }

        switch (kind) {
            case ZELDA64_DMA_EMPTY:
                logf_progress("[%4zu/%4zu] Skipping empty file", i + 1, in_rom.dma_info.count);
                out_rom.dma_table[i] = in_entry;
                break;

            case ZELDA64_DMA_DELETED:
                logf_progress("[%4zu/%4zu] Skipping deleted file", i + 1, in_rom.dma_info.count);

                out_rom.dma_table[i] = in_entry;
                break;

            case ZELDA64_DMA_UNCOMPRESSED: {
                logf_progress("[%4zu/%4zu] Compressing file", i + 1, in_rom.dma_info.count);

                result = zelda64_compress_file(&out_rom, rom_offset, &in_rom, i);
                if (result != ZELDA64_OK) {
                    exit_code = result_error(result, "failed to compress file");
                    goto cleanup_out_rom;
                }

                break;
            }

            default:
                abort();
        }

        struct zelda64_dma_entry const out_entry = out_rom.dma_table[i];
        if (out_entry.rom_end == UINT32_MAX) {
            // do nothing
        } else if (out_entry.rom_end == 0x0) {
            rom_offset += out_entry.vrom_end - out_entry.vrom_start;
        } else {
            rom_offset += out_entry.rom_end - out_entry.rom_start;
        }
    }

    result = zelda64_write_dmadata_to_rom(&out_rom);
    if (result != ZELDA64_OK) {
        exit_code = result_error(result, "failed to write DMADATA");
        goto cleanup_out_rom;
    }

    // Recalculate the check code and write out the header.
    result = zelda64_finalize_rom(&out_rom);
    if (result != ZELDA64_OK) {
        exit_code = result_error(result, "failed to write ROM header");
        goto cleanup_out_rom;
    }

    clock_t const end = clock();
    clock_t const delta = end - start;
    double const elapsed = (double) delta / CLOCKS_PER_SEC;

    logf_info("Decompression finished in %.2f seconds", elapsed);
    logf_info("Decompressed ROM check code is %016"PRIX64, out_rom.info.header.check_code);

cleanup_out_rom:
    zelda64_close_rom(&out_rom);
cleanup_in_rom:
    zelda64_close_rom(&in_rom);
    return exit_code;
}
