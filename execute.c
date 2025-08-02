/* Copyright (c) 2018, 2019, 2020, 2021, 2022 Dennis Wölfing
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

/* sh/execute.c
 * Shell command execution.
 */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "builtins.h"
#include "execute.h"
#include "expand.h"
#include "match.h"
#include "sh.h"
#include "trap.h"
#include "variables.h"

struct Function** functions;
size_t numFunctions;

unsigned long loopCounter;
unsigned long numBreaks;
unsigned long numContinues;
bool returning;
int returnStatus;

struct SavedFd {
    int fd;
    int saved;
    struct SavedFd* prev;
};

static struct SavedFd* savedFds;

static int executeCommand(struct Command* command, bool subshell);
static int executeCompoundCommand(struct Command* command, bool subshell);
static int executeFor(struct ForClause* clause);
static int executeFunction(struct Function* function, int argc, char** argv);
static int executeCase(struct CaseClause* clause);
static int executeList(struct List* list);
static int executePipeline(struct Pipeline* pipeline);
static int executeSimpleCommand(struct SimpleCommand* simpleCommand,
        bool subshell);
static bool performRedirection(struct Redirection* redirection, bool noSave);
static bool performRedirections(struct Redirection* redirections,
        size_t numRedirections, bool noSave);
static void popRedirection(void);
static int waitForCommand(pid_t pid);

int execute(struct CompleteCommand* command) {
    command->prevCommand = currentCommand;
    currentCommand = command;
    int result = executeList(&command->list);
    currentCommand = command->prevCommand;
    command->prevCommand = NULL;
    if (returning) {
        returning = false;
        return returnStatus;
    }
    return result;
}

int executeAndRead(struct CompleteCommand* command, struct StringBuffer* sb) {
    int pipeFds[2];
    if (pipe(pipeFds) < 0) err(1, "pipe");

    pid_t pid = fork();
    if (pid < 0) {
        err(1, "fork");
    } else if (pid == 0) {
        close(pipeFds[0]);
        if (!moveFd(pipeFds[1], 1)) err(1, "cannot move file descriptor");

        resetTraps();
        exitShell(execute(command));
    } else {
        close(pipeFds[1]);

        while (true) {
            char buffer[4096 + 1];

            ssize_t bytesRead = read(pipeFds[0], buffer, sizeof(buffer) - 1);
            if (bytesRead < 0) {
                err(1, "read");
            } else if (bytesRead == 0) {
                break;
            } else {
                buffer[bytesRead] = '\0';
                appendStringToStringBuffer(sb, buffer);
            }
        }

        return waitForCommand(pid);
    }
}

static int executeList(struct List* list) {
    for (size_t i = 0; i < list->numPipelines; i++) {
        lastStatus = executePipeline(&list->pipelines[i]);
        if (returning || numBreaks || numContinues) return 0;
        while (list->separators[i] == LIST_AND && lastStatus != 0) i++;
        while (list->separators[i] == LIST_OR && lastStatus == 0) i++;
    }
    return lastStatus;
}

static int executePipeline(struct Pipeline* pipeline) {
    if (pipeline->numCommands <= 1) {
        return executeCommand(&pipeline->commands[0], false);
    }

    int inputFd = -1;
    pid_t pgid = -1;

    int pgidPipe[2];
    if (shellOptions.monitor) {
        if (pipe(pgidPipe) < 0) err(1, "pipe");
    }

    for (size_t i = 0; i < pipeline->numCommands; i++) {
        bool firstInPipeline = i == 0;
        bool lastInPipeline = i == pipeline->numCommands - 1;

        int pipeFds[2];
        if (!lastInPipeline) {
            if (pipe(pipeFds) < 0) err(1, "pipe");
        }

        pid_t pid = fork();

        if (pid < 0) {
            err(1, "fork");
        } else if (pid == 0) {
            resetTraps();

            if (shellOptions.monitor) {
                close(pgidPipe[1]);
            }

            if (!lastInPipeline) {
                close(pipeFds[0]);
            }

            if (!firstInPipeline) {
                if (!moveFd(inputFd, 0)) {
                    warn("cannot move file descriptor");
                    _Exit(126);
                }
            }

            if (!lastInPipeline) {
                if (!moveFd(pipeFds[1], 1)) {
                    warn("cannot move file descriptor");
                    _Exit(126);
                }
            }

            if (shellOptions.monitor) {
                setpgid(0, pgid == -1 ? 0 : pgid);

                if (firstInPipeline) {
                    if (inputIsTerminal) {
                        tcsetpgrp(0, getpgid(0));
                    }

                    // Wait until all processes in the pipeline have started.
                    char c;
                    read(pgidPipe[0], &c, 1);
                    close(pgidPipe[0]);
                }
            }

            resetSignals();
            exit(executeCommand(&pipeline->commands[i], true));
        } else {
            if (shellOptions.monitor && firstInPipeline) {
                close(pgidPipe[0]);
            }

            if (!lastInPipeline) {
                close(pipeFds[1]);
                if (!firstInPipeline) {
                    close(inputFd);
                    if (shellOptions.monitor) {
                        setpgid(pid, pgid);
                    }
                } else if (shellOptions.monitor) {
                    pgid = pid;
                    while (getpgid(pid) != pgid) {
                        sched_yield();
                    }
                }

                inputFd = pipeFds[0];
            } else {
                assert(inputFd != 0);
                close(inputFd);

                if (shellOptions.monitor) {
                    setpgid(pid, pgid);
                    // Close the pipe to inform the first process in the
                    // pipeline that all processes have started.
                    close(pgidPipe[1]);
                }

                int exitStatus = waitForCommand(pid);

                for (size_t j = 0; j < pipeline->numCommands - 1; j++) {
                    // Wait for all other commands of the pipeline.
                    int status;
                    wait(&status);
                }
                if (pipeline->bang) return !exitStatus;
                return exitStatus;
            }
        }
    }

    assert(false); // This should be unreachable.
}

static void addFunction(struct Function* function) {
    function->refcount++;
    for (size_t i = 0; i < numFunctions; i++) {
        if (strcmp(functions[i]->name, function->name) == 0) {
            freeFunction(functions[i]);
            functions[i] = function;
            return;
        }
    }

    addToArray((void**) &functions, &numFunctions, &function,
            sizeof(struct Function*));
}

static int executeCommand(struct Command* command, bool subshell) {
    if (!executingTrap) {
        executeTraps();
    }

    if (subshell) {
        shellOptions.monitor = false;
    }

    if (command->type == COMMAND_SIMPLE) {
        return executeSimpleCommand(&command->simpleCommand, subshell);
    } else if (command->type == COMMAND_FUNCTION_DEFINITION) {
        addFunction(command->function);
        return 0;
    } else {
        for (size_t i = 0; i < command->numRedirections; i++) {
            struct Redirection redirection = command->redirections[i];
            if (redirection.type != REDIR_HERE_DOC_QUOTED) {
                int flags = redirection.type == REDIR_HERE_DOC ?
                        EXPAND_NO_QUOTES : 0;
                redirection.filename = expandWord2(redirection.filename, flags);
                if (!redirection.filename) {
                    for (; i > 0; i--) {
                        popRedirection();
                    }
                    return 1;
                }
            }
            if (!performRedirection(&redirection, false)) {
                if (redirection.type != REDIR_HERE_DOC_QUOTED) {
                    free((char*) redirection.filename);
                }
                for (; i > 0; i--) {
                    popRedirection();
                }
                return 1;
            }
            if (redirection.type != REDIR_HERE_DOC_QUOTED) {
                free((char*) redirection.filename);
            }
        }

        int status = executeCompoundCommand(command, subshell);

        for (size_t i = 0; i < command->numRedirections; i++) {
            popRedirection();
        }
        return status;
    }
}

static int executeCompoundCommand(struct Command* command, bool subshell) {
    int status = 0;
    switch (command->type) {
    case COMMAND_SUBSHELL:
        if (!subshell) {
            pid_t pid = fork();
            if (pid < 0) {
                err(1, "fork");
            } else if (pid == 0) {
                resetTraps();
                exit(executeList(&command->compoundList));
            } else {
                return waitForCommand(pid);
            }
        }
        return executeList(&command->compoundList);
    case COMMAND_BRACE_GROUP:
        return executeList(&command->compoundList);
    case COMMAND_FOR:
        return executeFor(&command->forClause);
    case COMMAND_CASE:
        return executeCase(&command->caseClause);
    case COMMAND_IF:
        for (size_t i = 0; i < command->ifClause.numConditions; i++) {
            if (executeList(&command->ifClause.conditions[i]) == 0) {
                if (returning || numBreaks || numContinues) return 0;
                return executeList(&command->ifClause.bodies[i]);
            }
            if (returning || numBreaks || numContinues) return 0;
        }
        if (command->ifClause.hasElse) {
            return executeList(&command->ifClause.bodies[
                    command->ifClause.numConditions]);
        }
        return 0;
    case COMMAND_WHILE:
    case COMMAND_UNTIL: {
        bool isUntil = command->type == COMMAND_UNTIL;
        loopCounter++;
        while (true) {
            bool condition = executeList(&command->loop.condition) == 0;
            if (returning) break;
            if (numBreaks) {
                numBreaks--;
                break;
            }
            if (numContinues) {
                numContinues--;
                if (numContinues) break;
                continue;
            }
            if (condition == isUntil) break;

            status = executeList(&command->loop.body);

            if (returning) break;
            if (numBreaks) {
                numBreaks--;
                break;
            }
            if (numContinues) {
                numContinues--;
                if (numContinues) break;
                continue;
            }
        }

        loopCounter--;
        return status;
    }
    default:
        assert(false);
    }
}

static int executeFor(struct ForClause* clause) {
    char** items = NULL;
    size_t numItems = 0;
    for (size_t i = 0; i < clause->numWords; i++) {
        char** fields;
        ssize_t numFields = expand(clause->words[i], EXPAND_PATHNAMES, &fields);
        if (numFields < 0) {
            for (size_t j = 0; j < numItems; j++) {
                free(items[j]);
            }
            free(items);
            return 1;
        }
        addMultipleToArray((void**) &items, &numItems, fields, sizeof(char*),
                numFields);
        free(fields);
    }

    loopCounter++;
    int status = 0;
    size_t i;
    for (i = 0; i < numItems; i++) {
        setVariable(clause->name, items[i], false);
        status = executeList(&clause->body);
        if (returning) break;
        if (numBreaks) {
            numBreaks--;
            break;
        }
        if (numContinues) {
            numContinues--;
            if (numContinues) break;
        }
        free(items[i]);
    }
    loopCounter--;

    for (; i < numItems; i++) {
        free(items[i]);
    }
    free(items);
    return status;
}

static int executeFunction(struct Function* function, int argc, char** argv) {
    char** newArguments = malloc((argc + 1) * sizeof(char*));
    if (!newArguments) err(1, "malloc");
    newArguments[0] = arguments[0];
    for (int i = 1; i < argc; i++) {
        newArguments[i] = strdup(argv[i]);
        if (!newArguments[i]) err(1, "malloc");
    }

    char** oldArguments = arguments;
    size_t oldNumArguments = numArguments;

    arguments = newArguments;
    numArguments = argc - 1;

    function->refcount++;
    int result = executeCommand(&function->body, false);
    freeFunction(function);

    for (int i = 1; i <= numArguments; i++) {
        free(arguments[i]);
    }
    free(arguments);

    arguments = oldArguments;
    numArguments = oldNumArguments;

    if (returning) {
        returning = false;
        return returnStatus;
    }
    return result;
}

static int executeCase(struct CaseClause* clause) {
    char* word = expandWord(clause->word);
    if (!word) return 1;

    int status = 0;
    for (size_t i = 0; i < clause->numItems; i++) {
        struct CaseItem* item = &clause->items[i];

        for (size_t j = 0; j < item->numPatterns; j++) {
            if (matchesPattern(word, item->patterns[j])) {
                if (item->hasList) {
                    status = executeList(&item->list);
                }
                if (item->fallthrough && !returning && !numBreaks &&
                        !numContinues) {
                    for (i = i + 1; i < clause->numItems; i++) {
                        item = &clause->items[i];
                        if (item->hasList) {
                            status = executeList(&item->list);
                        }
                        if (!item->fallthrough) break;
                    }
                }
                free(word);
                return status;
            }
        }
    }
    free(word);
    return status;
}

static bool isDeclarationUtility(char** words, size_t numWords) {
    if (numWords == 0) return false;
    if (strcmp(words[0], "export") == 0) {
        return true;
    }
    return false;
}

static int executeSimpleCommand(struct SimpleCommand* simpleCommand,
        bool subshell) {
    int result = 1;

    size_t numRedirections = simpleCommand->numRedirections;
    struct Redirection* redirections = calloc(numRedirections,
            sizeof(struct Redirection));
    if (!redirections) err(1, "malloc");
    size_t numAssignments = simpleCommand->numAssignmentWords;
    char** assignments = calloc(numAssignments, sizeof(char*));
    if (!assignments) err(1, "malloc");

    bool declUtility = isDeclarationUtility(simpleCommand->words,
            simpleCommand->numWords);

    char** arguments = NULL;
    size_t numArguments = 0;
    for (size_t i = 0; i < simpleCommand->numWords; i++) {
        char** fields;
        int flags = EXPAND_PATHNAMES;
        if (declUtility) {
            char* equals = strchr(simpleCommand->words[i], '=');
            if (equals) {
                *equals = '\0';
                if (isRegularVariableName(simpleCommand->words[i])) {
                    flags = EXPAND_NO_FIELD_SPLIT;
                }
                *equals = '=';
            }
        }

        ssize_t numFields = expand(simpleCommand->words[i], flags, &fields);
        if (numFields < 0) goto cleanup;
        addMultipleToArray((void**) &arguments, &numArguments, fields,
                sizeof(char*), numFields);
        free(fields);
    }

    addToArray((void**) &arguments, &numArguments, &(void*){NULL},
            sizeof(char*));

    for (size_t i = 0; i < numRedirections; i++) {
        redirections[i] = simpleCommand->redirections[i];
        if (redirections[i].type != REDIR_HERE_DOC_QUOTED) {
            int flags = redirections[i].type == REDIR_HERE_DOC ?
                    EXPAND_NO_QUOTES : 0;
            redirections[i].filename = expandWord2(redirections[i].filename,
                    flags);
            if (!redirections[i].filename) goto cleanup;
        }
    }

    for (size_t i = 0; i < numAssignments; i++) {
        assignments[i] = expandWord(simpleCommand->assignmentWords[i]);
        if (!assignments[i]) goto cleanup;
    }

    int argc = numArguments - 1;
    const char* command = arguments[0];
    const struct builtin* builtin = NULL;
    struct Function* function = NULL;
    if (command) {
        // Check for built-ins.
        for (const struct builtin* b = builtins; b->name; b++) {
            if (strcmp(command, b->name) == 0) {
                builtin = b;
                break;
            }
        }
        if (!builtin || !(builtin->flags & BUILTIN_SPECIAL)) {
            for (size_t i = 0; i < numFunctions; i++) {
                if (strcmp(command, functions[i]->name) == 0) {
                    builtin = NULL;
                    function = functions[i];
                    break;
                }
            }
        }
    } else {
        builtin = &builtins[0]; // the : builtin
    }

    if (builtin || function) {
        for (size_t i = 0; i < numAssignments; i++) {
            char* equals = strchr(assignments[i], '=');
            *equals = '\0';
            if (!builtin || !(builtin->flags & BUILTIN_SPECIAL)) {
                pushVariable(assignments[i], equals + 1);
            } else {
                setVariable(assignments[i], equals + 1, false);
            }
            free(assignments[i]);
        }
        free(assignments);
        assignments = NULL;
        numAssignments = 0;
    }

    if (!builtin && !function && !subshell) {
        pid_t pid = fork();

        if (pid < 0) {
            err(1, "fork");
        } else if (pid == 0) {
            if (shellOptions.monitor) {
                setpgid(0, 0);
                if (inputIsTerminal) {
                    tcsetpgrp(0, getpgid(0));
                }
            }

            resetSignals();
        } else {
            result = waitForCommand(pid);
            goto cleanup;
        }
    }

    bool noSave = builtin && strcmp(builtin->name, "exec") == 0;
    if (!performRedirections(redirections, numRedirections, noSave)) {
        if (!builtin) _Exit(1);
        result = 1;
        goto cleanup;
    }

    for (size_t i = 0; i < numRedirections; i++) {
        if (redirections[i].type != REDIR_HERE_DOC_QUOTED) {
            free((void*) redirections[i].filename);
        }
    }
    free(redirections);
    redirections = NULL;

    if (builtin) {
        result = builtin->func(argc, arguments);
    } else if (function) {
        result = executeFunction(function, argc, arguments);
    } else {
        executeUtility(argc, arguments, assignments, numAssignments);
    }

    if (!noSave) {
        for (size_t i = 0; i < numRedirections; i++) {
            popRedirection();
        }
    }

cleanup:
    if (subshell) _Exit(result);
    popVariables();
    for (size_t i = 0; i < numArguments; i++) {
        free(arguments[i]);
    }
    free(arguments);
    if (redirections) {
        for (size_t i = 0; i < numRedirections; i++) {
            if (redirections[i].type != REDIR_HERE_DOC_QUOTED) {
                free((void*) redirections[i].filename);
            }
        }
        free(redirections);
    }
    if (assignments) {
        for (size_t i = 0; i < numAssignments; i++) {
            free(assignments[i]);
        }
        free(assignments);
    }
    return result;
}

noreturn void executeUtility(int argc, char** arguments, char** assignments,
        size_t numAssignments) {
    const char* command = arguments[0];

    for (size_t i = 0; i < numAssignments; i++) {
        char* equals = strchr(assignments[i], '=');
        *equals = '\0';
        if (setenv(assignments[i], equals + 1, 1) < 0) {
            _Exit(126);
        }
        free(assignments[i]);
    }
    free(assignments);

    if (!command) _Exit(0);

    if (!strchr(command, '/')) {
        command = getExecutablePath(command, true);
    }

    if (command) {
        execv(command, arguments);

        if (errno == ENOEXEC) {
            arguments[0] = (char*) command;
            executeScript(argc, arguments);
        }

        warn("execv: '%s'", command);
        _Exit(126);
    } else {
        warnx("'%s': Command not found", arguments[0]);
        _Exit(127);
    }
}

char* getExecutablePath(const char* command, bool checkExecutable) {
    size_t commandLength = strlen(command);
    const char* path = getVariable("PATH");
    if (!path) return NULL;

    while (true) {
        size_t length = strcspn(path, ":");
        size_t prefixLength = length + 1 + (length == 0);
        char* buffer = malloc(commandLength + prefixLength + 1);
        if (!buffer) {
            warn("malloc");
            _Exit(126);
        }

        if (length == 0) {
            buffer[0] = '.';
            buffer[1] = '/';
        } else {
            memcpy(buffer, path, length);
            buffer[length] = '/';
        }
        memcpy(buffer + prefixLength, command, commandLength);
        buffer[commandLength + prefixLength] = '\0';

        if (access(buffer, checkExecutable ? X_OK : F_OK) == 0) {
            return buffer;
        }

        free(buffer);
        if (!path[length]) break;
        path += length + 1;
    }

    return NULL;
}

static int open_noclobber(const char* path) {
#ifdef O_NOCLOBBER
    return open(path, O_WRONLY | O_CREAT | O_NOCLOBBER, 0666);
#else
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        errno = EEXIST;
        return -1;
    }

    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        fstat(fd, &st);
        if (S_ISREG(st.st_mode)) {
            close(fd);
            errno = EEXIST;
            return -1;
        }
        return fd;
    }

    // This may spuriously fail with EEXIST if a non-regular file is created
    // concurrently. This failure is allowed by POSIX.
    return open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
#endif
}

static bool performRedirection(struct Redirection* redirection, bool noSave) {
    if (redirection->fd >= 10) {
        errno = EBADF;
        warn("'%d'", redirection->fd);
        return false;
    }

    int openFlags = 0;
    switch (redirection->type) {
    case REDIR_INPUT:
        openFlags = O_RDONLY;
        break;
    case REDIR_OUTPUT:
    case REDIR_OUTPUT_CLOBBER:
        openFlags = O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case REDIR_APPEND:
        openFlags = O_WRONLY | O_CREAT | O_APPEND;
        break;
    case REDIR_DUP:
        break;
    case REDIR_READ_WRITE:
        openFlags = O_RDWR | O_CREAT;
        break;
    case REDIR_HERE_DOC:
    case REDIR_HERE_DOC_QUOTED:
        break;
    default:
        assert(false);
    }

    int fd;
    if (redirection->type == REDIR_DUP) {
        if (strcmp(redirection->filename, "-") == 0) {
            fd = -1;
        } else {
            char* tail;
            fd = strtol(redirection->filename, &tail, 10);
            if (*tail || fd < 0 || fd >= 10) {
                errno = EBADF;
                warn("'%s'", redirection->filename);
                return false;
            }
            // Check that the file descriptor is valid.
            if (fcntl(fd, F_GETFL) < 0) {
                warn("'%s'", redirection->filename);
                return false;
            }
        }
    } else if (redirection->type == REDIR_HERE_DOC ||
            redirection->type == REDIR_HERE_DOC_QUOTED) {
        int pfd[2];
        if (pipe(pfd) < 0) err(1, "pipe");

        pid_t pid = fork();
        if (pid < 0) {
            err(1, "fork");
        } else if (pid == 0) {
            close(pfd[0]);
            size_t length = strlen(redirection->filename);
            size_t written = 0;
            while (written < length) {
                ssize_t bytesWritten = write(pfd[1],
                        redirection->filename + written, length - written);
                if (bytesWritten < 0) err(1, "write");
                written += bytesWritten;
            }
            exit(0);
        } else {
            close(pfd[1]);
            fd = pfd[0];
        }
    } else {
        if (redirection->type == REDIR_OUTPUT && shellOptions.noclobber) {
            fd = open_noclobber(redirection->filename);
        } else {
            fd = open(redirection->filename, openFlags, 0666);
        }
        if (fd < 0) {
            warn("open: '%s'", redirection->filename);
            return false;
        }
    }

    if (noSave) {
        if (fd != redirection->fd) {
            close(redirection->fd);
            if (fd != -1) {
                if (dup2(fd, redirection->fd) < 0) err(1, "dup2");

                if (redirection->type != REDIR_DUP) {
                    close(fd);
                }
            }
        }
        return true;
    }

    struct SavedFd* sfd = malloc(sizeof(struct SavedFd));
    if (!sfd) err(1, "malloc");
    sfd->fd = redirection->fd;
    if (fd != redirection->fd) {
        sfd->saved = fcntl(redirection->fd, F_DUPFD_CLOEXEC, 10);
        if (sfd->saved < 0) {
            if (errno != EBADF) {
                warn("failed to duplicate file descriptor");
                if (redirection->type != REDIR_DUP) {
                    close(fd);
                }
                free(sfd);
                return false;
            }
        } else {
            close(redirection->fd);
        }
        if (fd != -1) {
            if (dup2(fd, redirection->fd) < 0) err(1, "dup2");

            if (redirection->type != REDIR_DUP) {
                close(fd);
            }
        }
    } else {
        if (redirection->type == REDIR_DUP) {
            sfd->fd = -1;
        }
        sfd->saved = -1;
    }

    sfd->prev = savedFds;
    savedFds = sfd;
    return true;
}

static bool performRedirections(struct Redirection* redirections,
        size_t numRedirections, bool noSave) {
    for (size_t i = 0; i < numRedirections; i++) {
        if (!performRedirection(&redirections[i], noSave)) {
            for (; i > 0 && !noSave; i--) {
                popRedirection();
            }
            return false;
        }
    }

    return true;
}

static void popRedirection(void) {
    struct SavedFd* sfd = savedFds;
    if (sfd->fd != -1) {
        close(sfd->fd);
        if (sfd->saved != -1) {
            dup2(sfd->saved, sfd->fd);
            close(sfd->saved);
        }
    }
    savedFds = sfd->prev;
    free(sfd);
}

void freeRedirections(void) {
    while (savedFds) {
        struct SavedFd* sfd = savedFds;
        if (sfd->saved != -1) {
            close(sfd->saved);
        }
        savedFds = sfd->prev;
        free(sfd);
    }
}

void unsetFunction(const char* name) {
    for (size_t i = 0; i < numFunctions; i++) {
        if (strcmp(functions[i]->name, name) == 0) {
            freeFunction(functions[i]);
            if (i != numFunctions - 1) {
                functions[i] = functions[numFunctions - 1];
            }
            numFunctions--;
            return;
        }
    }
}

void unsetFunctions(void) {
    for (size_t i = 0; i < numFunctions; i++) {
        freeFunction(functions[i]);
    }
    free(functions);
    functions = NULL;
    numFunctions = 0;
}

static int waitForCommand(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        err(1, "waitpid");
    }

    if (inputIsTerminal && shellOptions.monitor) {
        tcsetpgrp(0, getpgid(0));
    }

    if (WIFSIGNALED(status)) {
        int signum = WTERMSIG(status);
        if (shellOptions.interactive && signum == SIGINT) {
            fputc('\n', stderr);
        } else if (shellOptions.interactive) {
            fprintf(stderr, "%s\n", strsignal(signum));
        }
        return 128 + signum;
    }

    return WEXITSTATUS(status);
}
