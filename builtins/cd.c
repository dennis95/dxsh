/* Copyright (c) 2018, 2019, 2021, 2025 Dennis Wölfing
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

/* builtins/cd.c
 * Change working directory.
 */

#include <config.h>
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "builtins.h"
#include "../builtins.h"
#include "../variables.h"

char* pwd;

static char* getNewLogicalPwd(const char* oldPwd, const char* dir) {
    if (*dir == '/') {
        oldPwd = "/";
    }

    // The resulting string cannot be longer than this.
    size_t newSize = strlen(oldPwd) + strlen(dir) + 2;
    char* newPwd = malloc(newSize);
    if (!newPwd) return NULL;

    char* pwdEnd = stpcpy(newPwd, oldPwd);
    const char* component = dir;
    size_t componentLength = strcspn(component, "/");

    while (*component) {
        if (componentLength == 0 ||
                (componentLength == 1 && strncmp(component, ".", 1) == 0)) {
            // We can ignore this path component.
        } else if (componentLength == 2 && strncmp(component, "..", 2) == 0) {
            char* lastSlash = strrchr(newPwd, '/');
            if (lastSlash == newPwd) {
                pwdEnd = newPwd + 1;
            } else {
                assert(lastSlash != NULL);
                pwdEnd = lastSlash;
            }
        } else {
            if (pwdEnd != newPwd + 1) {
                *pwdEnd++ = '/';
            }
            memcpy(pwdEnd, component, componentLength);
            pwdEnd += componentLength;
        }

        component += componentLength;
        if (*component) component++;
        componentLength = strcspn(component, "/");
        *pwdEnd = '\0';
    }

    return newPwd;
}

int cd(int argc, char* argv[]) {
    bool logical = true;
    bool errorPwd = false;

    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') break;
        if (argv[i][1] == '-' && argv[i][2] == '\0') {
            i++;
            break;
        }
        for (size_t j = 1; argv[i][j]; j++) {
            if (argv[i][j] == 'e') {
                errorPwd = true;
            } else if (argv[i][j] == 'L') {
                logical = true;
            } else if (argv[i][j] == 'P') {
                logical = false;
            } else {
                warnx("cd: invalid option '-%c'", argv[i][j]);
                return 2;
            }
        }

    }

    if (i + 1 < argc) {
        warnx("cd: extra operand");
        return 2;
    }

    const char* directory;
    bool printPwd = false;

    if (i < argc) {
        directory = argv[i];
        if (!*directory) {
            warnx("cd: invalid operand ''");
            return 2;
        } else if (strcmp(directory, "-") == 0) {
            directory = getVariable("OLDPWD");
            printPwd = true;
            if (!directory || !*directory) {
                warnx("cd: OLDPWD undefined");
                return 2;
            }
        }
    } else {
        directory = getVariable("HOME");
        if (!directory || !*directory) {
            warnx("cd: HOME unset or empty");
            return 2;
        }
    }

    void* toBeFreed = NULL;

    const char* curpath = directory;
    if (directory[0] != '/') {
        size_t firstComponentLength = strcspn(directory, "/");
        if (strncmp(directory, ".", firstComponentLength) != 0 &&
                strncmp(directory, "..", firstComponentLength) != 0) {
            const char* cdpath = getVariable("CDPATH");
            if (!cdpath) {
                cdpath = "";
            }
            size_t directoryLength = strlen(directory);
            bool end = false;

            while (!end) {
                size_t prefixLength = strcspn(cdpath, ":");
                const char* prefix = cdpath;
                cdpath += prefixLength;
                if (!*cdpath) {
                    end = true;
                }
                cdpath += 1;
                bool emptyPrefix = false;
                if (prefixLength == 0) {
                    prefix = ".";
                    prefixLength = 1;
                    emptyPrefix = true;
                }

                char* path = malloc(prefixLength + 1 + directoryLength + 1);
                if (!path) err(1, "malloc");
                stpcpy(stpcpy(mempcpy(path, prefix, prefixLength), "/"),
                        directory);

                struct stat st;
                if (stat(path, &st) == 0) {
                    if (S_ISDIR(st.st_mode)) {
                        toBeFreed = path;
                        curpath = path;
                        printPwd = !emptyPrefix;
                        break;
                    }
                }

                free(path);
            }
        }
    }

    char* oldPwd = pwd;
    int status = 0;

    if (logical && pwd) {
        char* newPwd = getNewLogicalPwd(pwd, curpath);
        free(toBeFreed);
        if (!newPwd || chdir(newPwd) < 0) {
            warn("cd: '%s'", curpath);
            free(newPwd);
            return 2;
        }

        pwd = newPwd;
    } else {
        if (chdir(curpath) < 0) {
            warn("cd: '%s'", curpath);
            free(toBeFreed);
            return 2;
        }

        free(toBeFreed);
        pwd = getcwd(NULL, 0);
        if (!pwd && errorPwd) {
            status = 1;
        }
    }

    if (printPwd && pwd) {
        puts(pwd);
        fflush(stdout);
    }

    if (pwd) {
        setVariable("PWD", pwd, true);
    } else {
        unsetVariable("PWD");
    }
    if (oldPwd) {
        setVariable("OLDPWD", oldPwd, true);
        free(oldPwd);
    } else {
        unsetVariable("OLDPWD");
    }
    return status;
}
