/* Copyright (c) 2018, 2025 Dennis Wölfing
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

/* builtins/umask.c
 * Set file creation mask.
 */

#include <config.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "builtins.h"

int sh_umask(int argc, char* argv[]) {
    // TODO: Implement the -S option.
    if (argc > 1) {
        char* end;
        unsigned long value = strtoul(argv[1], &end, 8);
        if (value > 0777 || *end) {
            errno = EINVAL;
            warn("umask: '%s'", argv[1]);
            return 1;
        }
        umask((mode_t) value);
    } else {
        mode_t mask = umask(0);
        umask(mask);
        printf("%.4o\n", (unsigned int) mask);
    }
    return 0;
}
