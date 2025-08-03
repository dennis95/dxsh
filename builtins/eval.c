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

/* builtins/eval.c
 * Evaluate command.
 */

#include <config.h>
#include <stdlib.h>

#include "builtins.h"
#include "../dxsh.h"
#include "../execute.h"
#include "../stringbuffer.h"

static bool readInputFromString(const char** str, bool newCommand,
        void* context) {
    (void) newCommand;

    const char** word = context;
    if (!*word) return false;
    *str = *word;
    *word = NULL;
    return true;
}

int eval(int argc, char* argv[]) {
    struct StringBuffer buffer;
    initStringBuffer(&buffer);

    for (int i = 1; i < argc; i++) {
        if (i != 1) {
            appendToStringBuffer(&buffer, ' ');
        }
        appendStringToStringBuffer(&buffer, argv[i]);
    }
    appendToStringBuffer(&buffer, '\n');

    char* string = finishStringBuffer(&buffer);
    struct Parser parser;
    const char* context = string;
    initParser(&parser, readInputFromString, &context);

    struct CompleteCommand command;
    enum ParserResult parserResult = parse(&parser, &command, true);

    int status = 0;
    if (parserResult == PARSER_MATCH) {
        status = execute(&command);
        freeCompleteCommand(&command);
    } else if (parserResult == PARSER_SYNTAX) {
        status = 1;
    }

    freeParser(&parser);
    free(string);
    return status;
}
