/*
 * log.h: logging utilities
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

#ifndef ZELDA64_LOG_H
#define ZELDA64_LOG_H

#include <stdio.h>

#define VERBOSITY_ALWAYS 0
#define VERBOSITY_INFO 1
#define VERBOSITY_PROGRESS 2

#define SEVERITY_INFO 1
#define SEVERITY_WARNING 2
#define SEVERITY_ERROR 3

extern char const* log_program;
extern int log_verbosity;

#if defined __GNUC__
#  define ZELDA64_PRINTF(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#else
#  define ZELDA64_PRINTF(fmt_index, first_arg)
#endif

void
log_message(FILE* stream, int level, int severity, char const* format, ...) ZELDA64_PRINTF(4, 5);

#define log_info(msg) log_message(stdout, VERBOSITY_INFO, SEVERITY_INFO, "%s", msg)
#define log_progress(msg) log_message(stdout, VERBOSITY_PROGRESS, SEVERITY_INFO, "%s", msg)
#define log_warning(msg) log_message(stderr, VERBOSITY_ALWAYS, SEVERITY_WARNING, "%s", msg)
#define log_error(msg) log_message(stderr, VERBOSITY_ALWAYS, SEVERITY_ERROR, "%s", msg)

#define logf_info(format, ...) log_message(stdout, VERBOSITY_INFO, SEVERITY_INFO, format, __VA_ARGS__)
#define logf_progress(format, ...) log_message(stdout, VERBOSITY_PROGRESS, SEVERITY_INFO, format, __VA_ARGS__)
#define logf_warning(format, ...) log_message(stderr, VERBOSITY_ALWAYS, SEVERITY_WARNING, format, __VA_ARGS__)
#define logf_error(format, ...) log_message(stderr, VERBOSITY_ALWAYS, SEVERITY_ERROR, format, __VA_ARGS__)

#endif //ZELDA64_LOG_H
