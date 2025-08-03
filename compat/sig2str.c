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

/* compat/sig2str.c
 * Translate a signal number to a signal name.
 */

#include <config.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "signalnames.h"
#include "../system.h"

int sig2str(int signum, char* str) {
    // Check that the number is a valid signal.
    sigset_t set;
    sigemptyset(&set);
    if (sigaddset(&set, signum) == -1) {
        return -1;
    }

    if (signum == SIGRTMIN) {
        strcpy(str, "RTMIN");
    } else if (signum == SIGRTMAX) {
        strcpy(str, "RTMAX");
    } else if (signum > SIGRTMIN && signum <= (SIGRTMIN + SIGRTMAX) / 2) {
        snprintf(str, SIG2STR_MAX, "RTMIN+%d", signum - SIGRTMIN);
    } else if (signum > (SIGRTMIN + SIGRTMAX) / 2 && signum < SIGRTMAX) {
        snprintf(str, SIG2STR_MAX, "RTMAX-%d", SIGRTMAX - signum);
    } else {
        for (const struct SignalName* name = compat_signames; name->number;
                name++) {
            if (name->number == signum) {
                strcpy(str, name->name);
                return 0;
            }
        }

        snprintf(str, SIG2STR_MAX, "%d", signum);
    }
    return 0;
}
