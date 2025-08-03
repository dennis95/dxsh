/* Copyright (c) 2023, 2025 Dennis Wölfing
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

/* builtins/read.c
 * Read input into variables.
 */

#include <config.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "../stringbuffer.h"
#include "../variables.h"

static bool isIfsWhiteSpace(char c, const char* ifs) {
    return (c == ' ' || c == '\t' || c == '\n') && strchr(ifs, c);
}

int sh_read(int argc, char* argv[]) {
    char delimiter = '\n';
    bool interpretBackslash = true;

    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') break;
        if (argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            break;
        }
        for (size_t j = 1; argv[i][j]; j++) {
            if (argv[i][j] == 'd') {
                const char* arg;
                if (argv[i][j + 1]) {
                    arg = &argv[i][j + 1];
                } else {
                    i++;
                    arg = argv[i];
                }

                if (!arg) {
                    warnx("read: option '-d' requires an argument");
                    return 2;
                } else if (strlen(arg) > 1) {
                    warnx("read: invalid delimiter '%s'", arg);
                    return 2;
                }

                delimiter = *arg;
                break;
            } else if (argv[i][j] == 'r') {
                interpretBackslash = false;
            } else {
                warnx("read: invalid option '-%c'", argv[i][j]);
                return 2;
            }
        }
    }

    if (i >= argc) {
        warnx("read: missing operand");
        return 2;
    }

    const char* ifs = getVariable("IFS");
    if (!ifs) {
        ifs = " \t\n";
    }

    bool delimiterFound = false;
    bool ignoreIfsAtBegin = false;
    bool eofReached = false;

    for (; i < argc; i++) {
        bool lastVar = i == argc - 1;
        struct StringBuffer buffer;
        initStringBuffer(&buffer);

        bool backslash = false;
        bool ignoreIfsWhitespaceAtBegin = true;

        while (!delimiterFound && !eofReached) {
            char c;
            ssize_t bytesRead = read(0, &c, 1);

            if (bytesRead < 0) {
                warn("read: read error");
                free(finishStringBuffer(&buffer));
                return 2;
            } else if (bytesRead == 0) {
                eofReached = true;
            } else /*if (bytesRead == 1)*/ {
                if (ignoreIfsWhitespaceAtBegin) {
                    if (c != delimiter && strchr(ifs, c)) {
                        if (c == ' ' || c == '\t' || c == '\n') {
                            continue;
                        } else if (ignoreIfsAtBegin) {
                            ignoreIfsAtBegin = false;
                            continue;
                        }

                    }

                    ignoreIfsAtBegin = false;
                    ignoreIfsWhitespaceAtBegin = false;
                }

                if (backslash) {
                    if (c != '\n') {
                        appendToStringBuffer(&buffer, c);
                    }
                    backslash = false;
                } else if (interpretBackslash && c == '\\') {
                    backslash = true;
                } else if (c == delimiter) {
                    delimiterFound = true;
                    break;
                } else if (!lastVar && strchr(ifs, c)) {
                    ignoreIfsAtBegin = c == ' ' || c == '\t' || c == '\n';
                    break;
                } else {
                    appendToStringBuffer(&buffer, c);
                }
            }
        }

        if (lastVar) {
            // Remove trailing IFS whitespace.
            while (buffer.used > 0 &&
                    isIfsWhiteSpace(buffer.buffer[buffer.used - 1], ifs)) {
                buffer.used--;
            }
        }

        char* value = finishStringBuffer(&buffer);
        setVariable(argv[i], value, false);
        free(value);
    }

    return eofReached ? 1 : 0;
}
