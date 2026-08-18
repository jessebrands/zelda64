/*
 * options.h: command line options parser
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

#ifndef ZELDA64_OPTIONS_H
#define ZELDA64_OPTIONS_H

#include <zelda64/zelda64.h>

#define ZELDA64_MAX_STAGES 8

enum zelda64_stage {
    ZELDA64_STAGE_DECOMPRESS,
};

struct zelda64_decompress_spec {
    enum zelda64_pack pack;
    enum zelda64_pad pad;
    char const* list_filename;
};

struct zelda64_stage_spec {
    enum zelda64_stage stage;
    union {
        struct zelda64_decompress_spec decompress;
    } as;
};

enum zelda64_parse_result {
    ZELDA64_PARSE_OK,
    ZELDA64_PARSE_HELP,
    ZELDA64_PARSE_VERSION,
    ZELDA64_PARSE_USAGE,
};

struct zelda64_options {
    struct zelda64_stage_spec stages[ZELDA64_MAX_STAGES];
    size_t stage_count;

    char const* in_filename;
    char const* out_filename;
};

enum zelda64_parse_result
parse_options(struct zelda64_options* options, int argc, char** argv);

#endif //ZELDA64_OPTIONS_H
