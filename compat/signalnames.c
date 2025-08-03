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

/* compat/signalnames.c
 * Signal names.
 */

#include <config.h>
#include <signal.h>
#include "signalnames.h"

#define SIG(name) {SIG ## name, #name}
const struct SignalName compat_signames[] = {
    // Signals defined by the C standard.
    SIG(ABRT),
    SIG(FPE),
    SIG(ILL),
    SIG(INT),
    SIG(SEGV),
    SIG(TERM),
    // Signals mandatory in POSIX.
    SIG(ALRM),
    SIG(BUS),
    SIG(CHLD),
    SIG(CONT),
    SIG(HUP),
    SIG(KILL),
    SIG(PIPE),
    SIG(QUIT),
    SIG(STOP),
    SIG(TSTP),
    SIG(TTIN),
    SIG(TTOU),
    SIG(USR1),
    SIG(USR2),
    SIG(URG),
    // Mandatory in POSIX.1-2024.
#ifdef SIGWINCH
    SIG(WINCH),
#endif
    // XSI signals.
#ifdef SIGSYS
    SIG(SYS),
#endif
#ifdef SIGTRAP
    SIG(TRAP),
#endif
#ifdef SIGVTALRM
    SIG(VTALRM),
#endif
#ifdef SIGXCPU
    SIG(XCPU),
#endif
#ifdef SIGXFSZ
    SIG(XFSZ),
#endif
    // Obsolete XSI signals.
#ifdef SIGPOLL
    SIG(POLL),
#endif
#ifdef SIGPROF
    SIG(PROF),
#endif
    // Signals found on some systems.
#ifdef SIGEMT
    SIG(EMT),
#endif
#ifdef SIGINFO
    SIG(INFO),
#endif
#ifdef SIGLOST
    SIG(LOST),
#endif
#ifdef SIGPWR
    SIG(PWR),
#endif
#ifdef SIGSTKFLT
    SIG(STKFLT),
#endif
    // Aliases found on some systems. These need to come after their proper
    // names, so that str2sig uses the proper names.
#ifdef SIGCLD
    SIG(CLD),
#endif
#ifdef SIGIO
    SIG(IO),
#endif
#ifdef SIGIOT
    SIG(IOT),
#endif
#ifdef SIGUNUSED
    SIG(UNUSED),
#endif
    { 0, "" }
};
