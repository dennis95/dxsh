/* Copyright (c) 2025 Dennis Wölfing
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

/* compat/tcgetwinsize.c
 * Get terminal window size.
 */

#include <config.h>
#include <errno.h>
#include <termios.h>
#include "../system.h"

#if HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

int tcgetwinsize(int fd, struct winsize* ws) {
#ifdef TIOCGWINSZ
    return ioctl(fd, TIOCGWINSZ, ws);
#else
    // Set some values, so that the code continues to work.
    ws->ws_col = 80;
    ws->ws_row = 25;
    errno = ENOSYS;
    return -1;
#endif
}
