/* Copyright (c) 2026 Dennis Wölfing
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

/* builtins/readonly.c
 * Make variables readonly.
 */

#include <config.h>
#include <err.h>
#include <string.h>

#include "builtins.h"
#include "../variables.h"

int readonly(int argc, char* argv[]) {
    bool print = false;
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') break;
        if (argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            break;
        }
        for (size_t j = 1; argv[i][j]; j++) {
            if (argv[i][j] == 'p') {
                print = true;
            } else {
                warnx("readonly: invalid option '-%c'", argv[i][j]);
                return 1;
            }
        }
    }

    if (print && i < argc) {
        warnx("readonly: extra operand '%s'", argv[i]);
        return 1;
    }

    if (print || i == argc) {
        printVariables("readonly ", VAR_READONLY);
        return 0;
    }

    bool success = true;
    for (; i < argc; i++) {
        char* equals = strchr(argv[i], '=');
        if (equals) *equals = '\0';

        if (!isRegularVariableName(argv[i])) {
            warnx("readonly: '%s' is not a valid name", argv[i]);
            success = false;
            continue;
        }
        if (!setVariable(argv[i], equals ? equals + 1 : NULL, VAR_READONLY)) {
            warnx("readonly: cannot set readonly variable '%s'", argv[i]);
            success = false;
        }
    }
    return success ? 0 : 1;
}
