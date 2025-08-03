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

/* builtins/return.c
 * Return from function.
 */

#include <config.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "builtins.h"
#include "../dxsh.h"
#include "../execute.h"

int sh_return(int argc, char* argv[]) {
    if (argc > 2) {
        warnx("return: too many arguments");
        returning = true;
        returnStatus = 1;
        return 1;
    }

    int status = lastStatus;

    if (argc == 2) {
        char* end;
        errno = 0;
        long value = strtol(argv[1], &end, 10);
        if (errno || value < INT_MIN || value > INT_MAX || *end) {
            warnx("return: invalid number '%s'", argv[1]);
            status = 1;
        } else {
            status = value;
        }
    }

    returning = true;
    returnStatus = status;
    return status;
}
