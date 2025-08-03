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

/* builtins/builtins.h
 * Shell builtins.
 */

#ifndef BUILTINS_BUILTINS_H
#define BUILTINS_BUILTINS_H

int sh_break(int argc, char* argv[]);
int cd(int argc, char* argv[]);
int colon(int argc, char* argv[]);
int sh_continue(int argc, char* argv[]);
int dot(int argc, char* argv[]);
int eval(int argc, char* argv[]);
int exec(int argc, char* argv[]);
int sh_exit(int argc, char* argv[]);
int export(int argc, char* argv[]);
int sh_read(int argc, char* argv[]);
int sh_return(int argc, char* argv[]);
int set(int argc, char* argv[]);
int shift(int argc, char* argv[]);
int sh_umask(int argc, char* argv[]);
int unset(int argc, char* argv[]);

#endif
