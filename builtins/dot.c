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

/* builtins/dot.c
 * Source a shell script.
 */

#include <config.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "../dxsh.h"
#include "../execute.h"

struct DotContext {
    FILE* file;
    char* buffer;
    size_t bufferSize;
};

static bool readInputFromFile(const char** str, bool newCommand,
        void* context) {
    (void) newCommand;
    struct DotContext* ctx = context;
    FILE* file = ctx->file;

    ssize_t length = getline(&ctx->buffer, &ctx->bufferSize, file);

    if (length < 0 && !feof(file)) err(1, "getline");
    if (length < 0) return false;

    *str = ctx->buffer;
    return true;
}

int dot(int argc, char* argv[]) {
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') break;
        if (argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            break;
        }
        warnx(".: invalid option '-%c'", argv[i][1]);
        return 1;
    }

    if (i >= argc) {
        warnx(".: missing file operand");
        return 1;
    }
    if (i + 1 < argc) {
        warnx(".: too many arguments");
        return 1;
    }

    char* pathname = argv[i];
    if (!strchr(pathname, '/')) {
        pathname = getExecutablePath(pathname, false);
        if (!pathname) {
            errno = ENOENT;
            warn(".: '%s'", argv[i]);
            return 1;
        }
    }

    FILE* file = fopen(pathname, "r");
    if (!file) {
        warn(".: '%s'", pathname);
        if (pathname != argv[i]) {
            free(pathname);
        }
        return 1;
    }
    if (pathname != argv[i]) {
        free(pathname);
    }

    struct DotContext context;
    context.file = file;
    context.buffer = NULL;
    context.bufferSize = 0;

    struct Parser parser;
    initParser(&parser, readInputFromFile, &context);
    struct CompleteCommand command;
    enum ParserResult parserResult = parse(&parser, &command, true);
    freeParser(&parser);
    free(context.buffer);
    fclose(file);

    int status = 1;
    if (parserResult == PARSER_MATCH) {
        status = execute(&command);
        freeCompleteCommand(&command);
    } else if (parserResult == PARSER_NO_CMD) {
        status = 0;
    }
    return status;
}
