/* Copyright (c) 2018, 2019, 2020, 2021, 2022, 2023, 2025 Dennis Wölfing
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

/* builtins.c
 * Shell builtins.
 */

#include <config.h>
#include <stddef.h>

#include "builtins.h"
#include "builtins/builtins.h"
#include "trap.h"

const struct builtin builtins[] = {
    { ":", colon, BUILTIN_SPECIAL }, // : must be the first entry in this list.
    { "break", sh_break, BUILTIN_SPECIAL },
    { "cd", cd, 0 },
    { "command", command, 0 },
    { "continue", sh_continue, BUILTIN_SPECIAL },
    { ".", dot, BUILTIN_SPECIAL },
    { "eval", eval, BUILTIN_SPECIAL },
    { "exec", exec, BUILTIN_SPECIAL },
    { "exit", sh_exit, BUILTIN_SPECIAL },
    { "export", export, BUILTIN_SPECIAL },
    { "read", sh_read, 0 },
    { "return", sh_return, BUILTIN_SPECIAL },
    { "set", set, BUILTIN_SPECIAL },
    { "shift", shift, BUILTIN_SPECIAL },
    { "trap", trap, BUILTIN_SPECIAL },
    { "umask", sh_umask, 0 },
    { "unset", unset, BUILTIN_SPECIAL },
    { NULL, NULL, 0 }
};
