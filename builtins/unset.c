/* Copyright (c) 2020, 2022, 2025 Dennis Wölfing
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

/* builtins/unset.c
 * Unset shell variables.
 */

#include <config.h>
#include <err.h>

#include "builtins.h"
#include "../execute.h"
#include "../variables.h"

int unset(int argc, char* argv[]) {
    bool function = false;
    bool variable = false;

    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') break;
        if (argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            break;
        }
        for (size_t j = 1; argv[i][j]; j++) {
            if (argv[i][j] == 'f') {
                function = true;
            } else if (argv[i][j] == 'v') {
                variable = true;
            } else {
                warnx("unset: invalid option '-%c'", argv[i][j]);
                return 1;
            }
        }
    }

    if (!function && !variable) {
        variable = true;
    }

    bool success = true;
    for (; i < argc; i++) {
        if (!isRegularVariableName(argv[i])) {
            warnx("unset: '%s' is not a valid name", argv[i]);
            success = false;
            continue;
        }
        if (variable) {
            unsetVariable(argv[i]);
        }
        if (function) {
            unsetFunction(argv[i]);
        }
    }
    return success ? 0 : 1;
}
