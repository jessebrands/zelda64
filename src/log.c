/*
 * log.c: logging utilities
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

#include <stdarg.h>

#include "log.h"

int log_verbosity = 0;

void
log_message(FILE* stream, int const level, int const severity, char const* format, ...) {
    if (log_verbosity < level && severity < SEVERITY_ERROR) {
        return;
    }

    if (severity >= SEVERITY_WARNING) {
        fprintf(stream, "zelda64: ");
    }

    switch (severity) {
        case SEVERITY_WARNING:
            fprintf(stream, "warn: ");
            break;

        case SEVERITY_ERROR:
            fprintf(stream, "error: ");
            break;

        default:
            break;
    }

    va_list arguments;
    va_start(arguments, format);
    vfprintf(stream, format, arguments);
    va_end(arguments);

    fprintf(stream, "\n");
}
