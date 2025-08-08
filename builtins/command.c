/* Copyright (c) 2025 Dennis Wölfing
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

/* builtins/command.c
 * Execute a simple command.
 */

#include <config.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "../builtins.h"
#include "../execute.h"

static const char* getStandardPath(void) {
#ifdef _CS_PATH
    static char* std_path = NULL;
    if (std_path) return std_path;

    size_t requiredLength = confstr(_CS_PATH, NULL, 0);
    std_path = malloc(requiredLength);
    if (!std_path) err(1, "malloc");
    confstr(_CS_PATH, std_path, requiredLength);
    return std_path;
#else
    return "/bin:/usr/bin";
#endif
}

int command(int argc, char* argv[]) {
    bool useStandardPath = false;
    bool print = false;
    bool printUser = false;

    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') break;
        if (argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            break;
        }
        for (size_t j = 1; argv[i][j]; j++) {
            if (argv[i][j] == 'p') {
                useStandardPath = true;
            } else if (argv[i][j] == 'v') {
                print = true;
            } else if (argv[i][j] == 'V') {
                printUser = true;
            } else {
                warnx("command: invalid option '-%c'", argv[i][j]);
                return 1;
            }
        }
    }

    if (print && printUser) {
        warnx("command: the '-v' and '-V' options are mutually exclusive");
        return 1;
    }

    const char* searchPath = NULL;
    if (useStandardPath) {
        searchPath = getStandardPath();
    }

    if (print || printUser) {
        if (i == argc) {
            warnx("command: missing operand");
            return 1;
        }

        if (i + 1 < argc) {
            warnx("command: too many arguments");
            return 1;
        }

        const char* command = argv[i];
        if (isReservedWord(command)) {
            if (print) {
                puts(command);
            } else {
                printf("%s is a shell reserved word\n", command);
            }
            return 0;
        }

        const struct builtin* builtin = NULL;
        struct Function* function = NULL;
        findBuiltinOrFunction(command, &builtin, &function);

        if (builtin || function) {
            if (print) {
                puts(command);
            } else if (builtin) {
                printf("%s is a shell %sbuiltin\n",
                    command,
                    builtin->flags & BUILTIN_SPECIAL ? "special " : "");
            } else {
                printf("%s is a shell function\n", command);
            }
        } else {
            char* toBeFreed = NULL;
            const char* path = NULL;

            if (!strchr(command, '/')) {
                toBeFreed = getExecutablePath(command, true, searchPath);
                path = toBeFreed;
            } else if (access(command, X_OK) == 0) {
                path = command;
            }

            if (path) {
                if (print) {
                    puts(path);
                } else {
                    printf("%s is %s\n", command, path);
                }
                free(toBeFreed);
            } else {
                if (printUser) {
                    warnx("command: '%s': not found", command);
                }
                return 1;
            }
        }

        return 0;
    }

    if (i == argc) {
        warnx("command: missing operand");
        return 1;
    }

    struct ExpandedSimpleCommand expandedCommand = {
        .arguments = argv + i,
        .numArguments = argc - i + 1,
        0
    };
    return executeExpandedCommand(&expandedCommand, false, false, searchPath);
}
