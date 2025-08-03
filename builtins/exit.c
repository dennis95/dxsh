/* Copyright (c) 2021, 2022, 2025 Dennis Wölfing
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

/* builtins/exit.c
 * Exit the shell.
 */

#include <config.h>
#include <err.h>
#include <limits.h>
#include <stdlib.h>

#include "builtins.h"
#include "../dxsh.h"
#include "../trap.h"

int sh_exit(int argc, char* argv[]) {
    if (argc > 2) {
        warnx("exit: too many arguments");
    }
    if (argc >= 2) {
        char* end;
        long value = strtol(argv[1], &end, 10);
        if (value < INT_MIN || value > INT_MAX || *end) {
            warnx("exit: invalid exit status '%s'", argv[1]);
            lastStatus = 255;
        } else {
            lastStatus = value;
        }
    }

    exitShell(lastStatus);
}
