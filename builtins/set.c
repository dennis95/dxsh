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

/* builtins/set.c
 * Set shell options.
 */

#include <config.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "../dxsh.h"
#include "../variables.h"

static void printOptionStatus(bool plusOption, const char* optionName,
        bool optionValue) {
    if (plusOption) {
        printf("set %co %s\n", optionValue ? '-' : '+', optionName);
    } else {
        printf("%-16s%s\n", optionName, optionValue ? "on" : "off");
    }
}

static void printOptions(bool plusOption) {
    printOptionStatus(plusOption, "allexport", shellOptions.allexport);
    printOptionStatus(plusOption, "errexit", shellOptions.errexit);
    printOptionStatus(plusOption, "hashall", shellOptions.hashall);
    printOptionStatus(plusOption, "ignoreeof", shellOptions.ignoreeof);
    printOptionStatus(plusOption, "monitor", shellOptions.monitor);
    printOptionStatus(plusOption, "noclobber", shellOptions.noclobber);
    printOptionStatus(plusOption, "noexec", shellOptions.noexec);
    printOptionStatus(plusOption, "noglob", shellOptions.noglob);
    printOptionStatus(plusOption, "nolog", shellOptions.nolog);
    printOptionStatus(plusOption, "notify", shellOptions.notify);
    printOptionStatus(plusOption, "nounset", shellOptions.nounset);
    printOptionStatus(plusOption, "verbose", shellOptions.verbose);
    printOptionStatus(plusOption, "vi", shellOptions.vi);
    printOptionStatus(plusOption, "xtrace", shellOptions.xtrace);
}

int set(int argc, char* argv[]) {
    bool setArguments = false;
    int i;
    for (i = 1; i < argc; i++) {
        if ((argv[i][0] != '-' && argv[i][0] != '+') || argv[i][1] == '\0') {
            break;
        }
        if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            setArguments = true;
            break;
        }

        const char* arg = argv[i];
        bool plusOption = arg[0] == '+';
        for (size_t j = 1; arg[j]; j++) {
            if (!handleShortOption(plusOption, arg[j])) {
                if (arg[j] != 'o') {
                    warnx("set: invalid option '%c%c'", arg[0], arg[j]);
                    return 1;
                }

                if (arg[j + 1]) {
                    warnx("set: unexpected '%c' after %co", arg[j + 1],
                            arg[0]);
                    return 1;
                }

                const char* option = argv[++i];
                if (!option) {
                    printOptions(plusOption);
                    return 0;
                }

                if (!handleLongOption(plusOption, option)) {
                    warnx("set: invalid option name '%s'", option);
                    return 1;
                }
                break;
            }
        }
    }

    if (argc == 1) {
        printVariables(false);
        return 0;
    }

    if (i < argc || setArguments) {
        int numArgs = argc - i;
        char** newArguments = malloc((numArgs + 1) * sizeof(char*));
        if (!newArguments) err(1, "malloc");
        newArguments[0] = arguments[0];
        for (int j = 0; j < numArgs; j++) {
            newArguments[j + 1] = strdup(argv[i + j]);
            if (!newArguments[j + 1]) err(1, "malloc");
        }

        for (int j = 1; j <= numArguments; j++) {
            free(arguments[j]);
        }
        free(arguments);
        arguments = newArguments;
        numArguments = numArgs;
    }

    return 0;
}
