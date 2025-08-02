/* Copyright (c) 2016, 2017, 2018, 2019 Dennis Wölfing
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

/* sh/sh.c
 * The shell.
 */

#include <assert.h>
#include <err.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "builtins.h"
#include "execute.h"
#include "expand.h"
#include "parser.h"
#include "tokenizer.h"

#ifndef DENNIX_VERSION
#  define DENNIX_VERSION ""
#endif

bool inputIsTerminal;
struct ShellOptions shellOptions;
struct termios termios;

static char** arguments;
static char* buffer;
static size_t bufferSize;
static struct CompleteCommand command;
static FILE* inputFile;
static jmp_buf jumpBuffer;
static struct Tokenizer tokenizer;

static void help(const char* argv0);
static int parseOptions(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    int optionIndex = parseOptions(argc, argv);
    int numArguments = argc - optionIndex;

    if (numArguments == 0) {
        arguments = malloc(2 * sizeof(char*));
        if (!arguments) err(1, "malloc");
        arguments[0] = strdup(argv[0]);
        if (!arguments[0]) err(1, "strdup");
        arguments[1] = NULL;
    } else {
        if (shellOptions.stdInput) {
            numArguments++;
            optionIndex--;
        }
        arguments = malloc((numArguments + 1) * sizeof(char*));
        if (!arguments) err(1, "malloc");
        if (shellOptions.stdInput) {
            arguments[0] = strdup(argv[0]);
            if (!arguments[0]) err(1, "strdup");
        }

        for (int i = shellOptions.stdInput; i < numArguments; i++) {
            arguments[i] = strdup(argv[optionIndex + i]);
            if (!arguments[i]) err(1, "strdup");
        }
        arguments[numArguments] = NULL;
    }

    pwd = getenv("PWD");
    if (pwd) {
        pwd = strdup(pwd);
    } else {
        pwd = getcwd(NULL, 0);
        if (pwd) {
            setenv("PWD", pwd, 1);
        }
    }

    inputFile = stdin;

    if (setjmp(jumpBuffer)) {
        shellOptions = (struct ShellOptions) {false};
        assert(arguments[0]);
    }

    if (!shellOptions.stdInput && arguments[0]) {
        inputFile = fopen(arguments[0], "r");
        if (!inputFile) err(1, "fopen: '%s'", arguments[0]);
    }

    // Ignore signals that should not terminate the (interactive) shell.
    if (shellOptions.interactive) {
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
    }

    inputIsTerminal = isatty(0);
    if (shellOptions.interactive && inputIsTerminal) {
        tcgetattr(0, &termios);
    }

    const char* username = getlogin();
    if (!username) {
        username = "?";
    }
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        strcpy(hostname, "?");
    }

    while (true) {
        if (shellOptions.interactive) {
            fprintf(stderr, "\e[32m%s@%s \e[1;36m%s $\e[22;39m ",
                    username, hostname, pwd ? pwd : ".");
        }

        enum TokenizerResult tokenResult;
        if (!initTokenizer(&tokenizer)) err(1, "initTokenizer");

continue_tokenizing:
        do {
            ssize_t length = getline(&buffer, &bufferSize, inputFile);
            if (length < 0) {
                putchar('\n');
                if (feof(inputFile)) exit(0);
                err(1, NULL);
            }

            tokenResult = splitTokens(&tokenizer, buffer);
            if (tokenResult == TOKENIZER_ERROR) {
                err(1, "Tokenizer error");
            } else if (tokenResult == TOKENIZER_NEED_INPUT &&
                    shellOptions.interactive) {
                fputs("> ", stderr);
            }
        } while (tokenResult == TOKENIZER_NEED_INPUT);

        if (tokenResult != TOKENIZER_DONE) {
            freeTokenizer(&tokenizer);
            continue;
        }

        if (tokenizer.numTokens == 1) {
            // Only a newline token was found.
            freeTokenizer(&tokenizer);
            continue;
        }

        struct Parser parser;
        initParser(&parser, &tokenizer);

        enum ParserResult parserResult = parse(&parser, &command);

        if (parserResult == PARSER_NEWLINE) {
            if (shellOptions.interactive) {
                fputs("> ", stderr);
            }
            goto continue_tokenizing;
        } else if (parserResult == PARSER_ERROR) {
            err(1, "Parser error");
        } else if (parserResult == PARSER_MATCH) {
            execute(&command);
            freeCompleteCommand(&command);
        }

        freeTokenizer(&tokenizer);
    }
}

noreturn void executeScript(char** argv) {
    // Reset all global state and jump back at the beginning of the shell to
    // execute the script.
    freeCompleteCommand(&command);
    freeTokenizer(&tokenizer);

    for (size_t i = 0; arguments[i]; i++) {
        free(arguments[i]);
    }
    free(arguments);
    arguments = argv;

    if (inputFile != stdin) {
        fclose(inputFile);
    }
    longjmp(jumpBuffer, 1);
}

static void help(const char* argv0) {
    printf("Usage: %s [OPTIONS] [COMMAND] [ARGUMENT...]\n"
            "  -i                       make shell interactive\n"
            "  -m, -o monitor           enable job control\n"
            "  -o OPTION                enable OPTION\n"
            "  -s                       read from stdin\n"
            "      --help               display this help\n"
            "      --version            display version info\n",
            argv0);
}

static int parseOptions(int argc, char* argv[]) {
    int i;
    for (i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (arg[0] != '-' && arg[0] != '+') break;

        bool plusOption = arg[0] == '+';
        if (!plusOption && (arg[1] == '\0' ||
                (arg[1] == '-' && arg[2] == '\0'))) {
            i++;
            break;
        }

        if (!plusOption && arg[1] == '-') {
            arg += 2;
            if (strcmp(arg, "help") == 0) {
                help(argv[0]);
                exit(0);
            } else if (strcmp(arg, "version") == 0) {
                printf("%s (Dennix) %s\n", argv[0], DENNIX_VERSION);
                exit(0);
            } else {
                errx(1, "unrecognized option '--%s'", arg);
            }
        } else {
            for (size_t j = 1; arg[j]; j++) {
                switch (arg[j]) {
                case 'a': shellOptions.allexport = !plusOption; break;
                case 'b': shellOptions.notify = !plusOption; break;
                case 'C': shellOptions.noclobber = !plusOption; break;
                case 'e': shellOptions.errexit = !plusOption; break;
                case 'f': shellOptions.noglob = !plusOption; break;
                case 'h': shellOptions.hashall = !plusOption; break;
                case 'm': shellOptions.monitor = !plusOption; break;
                case 'n': shellOptions.noexec = !plusOption; break;
                case 'u': shellOptions.nounset = !plusOption; break;
                case 'v': shellOptions.verbose = !plusOption; break;
                case 'x': shellOptions.xtrace = !plusOption; break;

                case 'o': {
                    if (arg[j + 1]) {
                        errx(1, "unexpected '%c' after %co", arg[j + 1],
                                arg[0]);
                    }

                    const char* option = argv[++i];
                    if (!option) {
                        errx(1, "%co requires an argument", arg[0]);
                    }

                    if (strcmp(option, "allexport") == 0) {
                        shellOptions.allexport = !plusOption;
                    } else if (strcmp(option, "errexit") == 0) {
                        shellOptions.errexit = !plusOption;
                    } else if (strcmp(option, "hashall") == 0) {
                        shellOptions.hashall = !plusOption;
                    } else if (strcmp(option, "ignoreeof") == 0) {
                        shellOptions.ignoreeof = !plusOption;
                    } else if (strcmp(option, "monitor") == 0) {
                        shellOptions.monitor = !plusOption;
                    } else if (strcmp(option, "noclobber") == 0) {
                        shellOptions.noclobber = !plusOption;
                    } else if (strcmp(option, "noexec") == 0) {
                        shellOptions.noexec = !plusOption;
                    } else if (strcmp(option, "noglob") == 0) {
                        shellOptions.noglob = !plusOption;
                    } else if (strcmp(option, "nolog") == 0) {
                        shellOptions.nolog = !plusOption;
                    } else if (strcmp(option, "notify") == 0) {
                        shellOptions.notify = !plusOption;
                    } else if (strcmp(option, "nounset") == 0) {
                        shellOptions.nounset = !plusOption;
                    } else if (strcmp(option, "verbose") == 0) {
                        shellOptions.verbose = !plusOption;
                    } else if (strcmp(option, "vi") == 0) {
                        shellOptions.vi = !plusOption;
                    } else if (strcmp(option, "xtrace") == 0) {
                        shellOptions.xtrace = !plusOption;
                    } else {
                        errx(1, "invalid option name '%s'", option);
                    }

                    goto nextArg;
                }

                // TODO: Add -c option
                case 'i':
                    if (!plusOption) {
                        shellOptions.interactive = true;
                        shellOptions.monitor = true;
                        break;
                    }
                    // fallthrough
                case 's':
                    if (!plusOption) {
                        shellOptions.stdInput = true;
                        break;
                    }
                    // fallthrough
                default:
                    errx(1, "invalid option '%c%c'", arg[0], arg[j]);
                }
            }
        }
nextArg:;
    }

    if (!shellOptions.command && i >= argc) {
        shellOptions.stdInput = true;
    }

    if (shellOptions.stdInput && isatty(0) && isatty(2)) {
        shellOptions.interactive = true;
        shellOptions.monitor = true;
    }

    return i;
}

// Utility functions:

bool addToArray(void** array, size_t* used, void* value, size_t size) {
    void* newArray = reallocarray(*array, size, *used + 1);
    if (!newArray) return false;
    *array = newArray;
    memcpy((void*) ((uintptr_t) *array + size * *used), value, size);
    (*used)++;
    return true;
}

bool moveFd(int old, int new) {
    if (dup2(old, new) < 0) return false;
    if (old != new) {
        close(old);
    }
    return true;
}
