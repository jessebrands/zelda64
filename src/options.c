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

#include <stdlib.h>

static struct zelda64_stage_spec*
init_decompress_stage(struct zelda64_stage_spec* spec) {
    *spec = (struct zelda64_stage_spec){
        .stage = ZELDA64_STAGE_DECOMPRESS,
        .as.decompress = {
            .pack = ZELDA64_PACK_SPARSE,
            .pad = ZELDA64_PAD_ZERO,
            .list_filename = NULL,
        },
    };
    return spec;
}

static struct zelda64_stage_spec*
push_stage(struct zelda64_options* opts, enum zelda64_stage const stage) {
    if (opts->stage_count == ZELDA64_MAX_STAGES) {
        log_error("too many operations");
        return NULL;
    }

    switch (stage) {
        case ZELDA64_STAGE_DECOMPRESS:
            return init_decompress_stage(&opts->stages[opts->stage_count++]);

        default:
            abort();
    }
}

static struct zelda64_stage_spec*
current_stage(struct zelda64_options* options) {
    return options->stage_count > 0 ? &options->stages[options->stage_count - 1] : NULL;
}

static int
parse_pack_value(enum zelda64_pack* pack, char const* arg) {
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
    log_error("expected one of: sparse, dense");
    return -1;
}

static int
parse_pack(struct zelda64_stage_spec* spec, char const* arg) {
    assert(arg != NULL);

    if (spec == NULL) {
        log_error("invalid option 'pack'");
        return -1;
    }
    if (spec->stage == ZELDA64_STAGE_DECOMPRESS) {
        return parse_pack_value(&spec->as.decompress.pack, arg);
    }

    log_error("option 'pack' invalid for stage");
    return -1;
}

static int
parse_pad_value(enum zelda64_pad* pad, char const* arg) {
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
    log_error("expected one of: none, zero, ramp");
    return -1;
}

static int
parse_pad(struct zelda64_stage_spec* spec, char const* arg) {
    assert(arg != NULL);

    if (spec == NULL) {
        log_error("invalid option 'pad'");
        return -1;
    }
    if (spec->stage == ZELDA64_STAGE_DECOMPRESS) {
        return parse_pad_value(&spec->as.decompress.pad, arg);
    }

    log_error("option 'pad' invalid for stage");
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
            if (strcmp(arg, "-") == 0) {
                log_error("stdio is not supported");
                return ZELDA64_PARSE_USAGE;
            }

            if (options->in_filename == NULL) {
                options->in_filename = arg;
            } else if (options->out_filename == NULL) {
                options->out_filename = arg;
            } else {
                logf_error("unexpected operand '%s'", arg);
                return ZELDA64_PARSE_USAGE;
            }
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

            if (is_option(opt, name_len, "decompress")) {
                if (attached != NULL) {
                    log_error("option --decompress takes no argument");
                    return ZELDA64_PARSE_USAGE;
                }
                if (push_stage(options, ZELDA64_STAGE_DECOMPRESS) == NULL) {
                    return ZELDA64_PARSE_USAGE;
                }
                continue;
            }
            if (is_option(opt, name_len, "pack")) {
                char const* const value = take_value(attached, &i, argc, argv);
                if (value == NULL || parse_pack(current_stage(options), value) != 0) {
                    return ZELDA64_PARSE_USAGE;
                }
                continue;
            }
            if (is_option(opt, name_len, "pad")) {
                char const* const value = take_value(attached, &i, argc, argv);
                if (value == NULL || parse_pad(current_stage(options), value) != 0) {
                    return ZELDA64_PARSE_USAGE;
                }
                continue;
            }
            if (is_option(opt, name_len, "version")) {
                if (attached != NULL) {
                    log_error("option --version takes no argument");
                    return ZELDA64_PARSE_USAGE;
                }
                return ZELDA64_PARSE_VERSION;
            }
            if (is_option(opt, name_len, "help")) {
                if (attached != NULL) {
                    log_error("option --help takes no argument");
                    return ZELDA64_PARSE_USAGE;
                }
                return ZELDA64_PARSE_HELP;
            }
            if (is_option(opt, name_len, "verbose")) {
                if (attached != NULL) {
                    log_error("option --verbose takes no argument");
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
                case 'd':
                    if (push_stage(options, ZELDA64_STAGE_DECOMPRESS) == NULL) {
                        return ZELDA64_PARSE_USAGE;
                    }
                    continue;

                case 'v':
                    log_verbosity++;
                    continue;

                case 'P': {
                    char const* const attached = (c[1] != '\0') ? c + 1 : NULL;
                    char const* const value = take_value(attached, &i, argc, argv);
                    if (value == NULL || parse_pack(current_stage(options), value) != 0) {
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
