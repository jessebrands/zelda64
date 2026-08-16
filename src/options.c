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

#include <stdbool.h>

#include "options.h"

#include <string.h>

#include "log.h"

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

            if (strcmp(opt, "version") == 0) {
                return ZELDA64_PARSE_VERSION;
            }
            if (strcmp(opt, "help") == 0) {
                return ZELDA64_PARSE_HELP;
            }
            if (strcmp(opt, "verbose") == 0) {
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

                default:
                    logf_error("unknown option -- '%c'", *c);
                    return ZELDA64_PARSE_USAGE;
            }
        }
    }

    return ZELDA64_PARSE_OK;
}
