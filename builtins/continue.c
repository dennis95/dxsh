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

/* builtins/continue.c
 * Continue next iteration of loop.
 */

#include <config.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>

#include "builtins.h"
#include "../execute.h"

int sh_continue(int argc, char* argv[]) {
    if (argc > 2) {
        warnx("continue: too many arguments");
        return 1;
    }
    unsigned long continues = 1;
    if (argc == 2) {
        char* end;
        errno = 0;
        continues = strtoul(argv[1], &end, 10);
        if (errno || continues == 0 || *end) {
            warnx("continue: invalid number '%s'", argv[1]);
            return 1;
        }
    }

    if (!loopCounter) {
        warnx("continue: used outside of loop");
        return 1;
    }

    numContinues = continues;
    if (numContinues > loopCounter) {
        numContinues = loopCounter;
    }
    return 0;
}
