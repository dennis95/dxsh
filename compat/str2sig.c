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

/* compat/str2sig.c
 * Translate a signal name to a signal number.
 */

#include <config.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "signalnames.h"

int str2sig(const char* str, int* num) {
    if (*str >= '0' && *str <= '9') {
        char* end;
        long n = strtol(str, &end, 10);
        if (n < 0 || n > INT_MAX || *end) {
            return -1;
        }

        sigset_t set;
        sigemptyset(&set);
        if (sigaddset(&set, n) == -1) {
            return -1;
        }

        *num = n;
        return 0;
    }

    if (strcmp(str, "RTMIN") == 0) {
        *num = SIGRTMIN;
        return 0;
    } else if (strcmp(str, "RTMAX") == 0) {
        *num = SIGRTMAX;
        return 0;
    } else if (strncmp(str, "RTMIN+", 6) == 0) {
        char* end;
        long n = strtol(str + 6, &end, 10);
        if (n < 1 || n >= SIGRTMAX - SIGRTMIN || *end) {
            return -1;
        }
        *num = SIGRTMIN + n;
        return 0;
    } else if (strncmp(str, "RTMAX-", 6) == 0) {
        char* end;
        long n = strtol(str + 6, &end, 10);
        if (n < 1 || n >= SIGRTMAX - SIGRTMIN || *end) {
            return -1;
        }
        *num = SIGRTMAX - n;
        return 0;
    }

    for (const struct SignalName* name = compat_signames; name->number;
            name++) {
        if (strcmp(str, name->name) == 0) {
            *num = name->number;
            return 0;
        }
    }

    return -1;
}
