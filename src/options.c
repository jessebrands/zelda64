/*
 * options.c: command line options parser
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

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "log.h"
#include "options.h"

static int
parse_pack(enum zelda64_pack* pack, char const* arg) {
    assert(pack != NULL);
    assert(arg != NULL);

    if (strcmp(arg, "sparse") == 0) {
        *pack = ZELDA64_PACK_SPARSE;
        return 0;
    }
    if (strcmp(arg, "dense") == 0) {
        *pack = ZELDA64_PACK_DENSE;
        return 0;
    }
    logf_error("invalid pack mode '%s'", arg);
    logf_error("expected one of: sparse, dense");
    return -1;
}

static int
parse_pad(enum zelda64_pad* pad, char const* arg) {
    assert(pad != NULL);
    assert(arg != NULL);

    if (strcmp(arg, "none") == 0) {
        *pad = ZELDA64_PAD_NONE;
        return 0;
    }
    if (strcmp(arg, "zero") == 0) {
        *pad = ZELDA64_PAD_ZERO;
        return 0;
    }
    if (strcmp(arg, "ramp") == 0) {
        *pad = ZELDA64_PAD_RAMP;
        return 0;
    }
    logf_error("invalid pad mode '%s'", arg);
    logf_error("expected one of: none, zero, ramp");
    return -1;
}

static bool
is_option(char const* opt, size_t const name_len, char const* const name) {
    return strlen(name) == name_len && strncmp(opt, name, name_len) == 0;
}

static char const*
take_value(char const* const attached, int* i, int argc, char** argv) {
    if (attached != NULL) {
        return attached;
    }
    if (*i + 1 >= argc) {
        return NULL;
    }
    return argv[++(*i)];
}

enum zelda64_parse_result
parse_options(struct zelda64_options* options, int argc, char** argv) {
    bool end = false;

    for (int i = 1; i < argc; ++i) {
        char const* arg = argv[i];

        // just a dash, or the end
        if (end || arg[0] != '-' || arg[1] == '\0') {
            continue;
        }

        // --long-argument
        if (arg[1] == '-') {
            // play nice with GNU, '--' means stop parsing
            if (arg[2] == '\0') {
                end = true;
                continue;
            }

            char const* opt = &arg[2];
            char const* const eq = strchr(opt, '=');
            size_t const name_len = (eq != NULL) ? (size_t) (eq - opt) : strlen(opt);
            char const* const attached = (eq != NULL) ? eq + 1 : NULL;

            if (is_option(opt, name_len, "pack")) {
                char const* const value = take_value(attached, &i, argc, argv);
                if (value == NULL || parse_pack(&options->pack, value) != 0) {
                    return ZELDA64_PARSE_USAGE;
                }
                continue;
            }
            if (is_option(opt, name_len, "pad")) {
                char const* const value = take_value(attached, &i, argc, argv);
                if (value == NULL || parse_pad(&options->pad, value) != 0) {
                    return ZELDA64_PARSE_USAGE;
                }
                continue;
            }
            if (is_option(opt, name_len, "version")) {
                if (attached != NULL) {
                    logf_error("option --version takes no argument");
                    return ZELDA64_PARSE_USAGE;
                }
                return ZELDA64_PARSE_VERSION;
            }
            if (is_option(opt, name_len, "help")) {
                if (attached != NULL) {
                    logf_error("option --help takes no argument");
                    return ZELDA64_PARSE_USAGE;
                }
                return ZELDA64_PARSE_HELP;
            }
            if (is_option(opt, name_len, "verbose")) {
                if (attached != NULL) {
                    logf_error("option --verbose takes no argument");
                    return ZELDA64_PARSE_USAGE;
                }
                log_verbosity++;
                continue;
            }

            logf_error("unknown option '%s'", arg);
            return ZELDA64_PARSE_USAGE;
        }

        // must be a -s -h -o -r -t argument
        for (char const* c = arg + 1; *c != '\0'; ++c) {
            switch (*c) {
                case 'v':
                    log_verbosity++;
                    continue;

                case 'P': {
                    char const* const attached = (c[1] != '\0') ? c + 1 : NULL;
                    char const* const value = take_value(attached, &i, argc, argv);
                    if (value == NULL || parse_pack(&options->pack, value) != 0) {
                        return ZELDA64_PARSE_USAGE;
                    }
                    break;
                }

                default:
                    logf_error("unknown option -- '%c'", *c);
                    return ZELDA64_PARSE_USAGE;
            }

            break;
        }
    }

    return ZELDA64_PARSE_OK;
}
