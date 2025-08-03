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
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
            } else if (lastSlash) {
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
    const char* newCwd;
    if (argc >= 2) {
        newCwd = argv[1];
    } else {
        newCwd = getVariable("HOME");
        if (!newCwd) {
            warnx("HOME not set");
            return 1;
        }
    }

    if (!pwd) {
        if (chdir(newCwd) < 0) {
            warn("cd: '%s'", newCwd);
            return 1;
        }

        pwd = getcwd(NULL, 0);
    } else {
        char* newPwd = getNewLogicalPwd(pwd, newCwd);
        if (!newPwd || chdir(newPwd) < 0) {
            warn("cd: '%s'", newCwd);
            free(newPwd);
            return 1;
        }

        free(pwd);
        pwd = newPwd;
    }

    if (pwd) {
        setVariable("PWD", pwd, true);
    } else {
        unsetVariable("PWD");
    }
    return 0;
}
