/* Copyright (c) 2022, 2025 Dennis Wölfing
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* builtins/shift.c
 * Shift command line arguments.
 */

#include <config.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>

#include "builtins.h"
#include "../variables.h"

int shift(int argc, char* argv[]) {
    if (argc > 2) {
        warnx("shift: too many arguments");
        return 1;
    }

    long n = 1;
    if (argc == 2) {
        char* end;
        errno = 0;
        n = strtol(argv[1], &end, 10);
        if (errno || n < 0 || *end) {
            warnx("shift: invalid number '%s'", argv[1]);
            return 1;
        }
    }

    if (n == 0) return 0;

    int newNumArguments = numArguments < n ? 0 : numArguments - n;
    char** newArguments = malloc((newNumArguments + 1) * sizeof(char*));
    if (!newArguments) err(1, "malloc");
    newArguments[0] = arguments[0];
    for (int i = 1; i <= newNumArguments; i++) {
        newArguments[i] = arguments[i + n];
    }

    for (int i = 1; i <= n && i <= numArguments; i++) {
        free(arguments[i]);
    }
    free(arguments);
    arguments = newArguments;
    numArguments = newNumArguments;

    return 0;
}
