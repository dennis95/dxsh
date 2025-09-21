# Copyright (c) 2025 Dennis Wölfing
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

. "${test_script_dir?}/libtest.sh"

# Tests for POSIX XSH 2.5 Parameters and Variables.

test_case 'parameters:positional'
test_shell_succeed -s one two three four five six seven eight nine ten eleven twelve << "EOF"
echo "$1 ${2} ${03} ${0000004} $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} $10"
EOF
assert_output << EOF
one two three four five six seven eight nine ten eleven twelve  one0
EOF

test_case 'parameters:@'
test_shell_succeed -s "a b c" d "e  f" << "EOF"
printf '[%s]' $@
printf '\n'
printf '[%s]' "$@"
printf '\n'
printf '[%s]' "x$@y"
printf '\n'
set --
printf '[%s]' x $@
printf '\n'
printf '[%s]' x "$@"
printf '\n'
printf '[%s]' x "x$@y"
printf '\n'
# TODO: printf '[%s]' x ''"$@"
# TODO: Check with one empty parameter expansion
EOF
assert_output << EOF
[a][b][c][d][e][f]
[a b c][d][e  f]
[xa b c][d][e  fy]
[x]
[x]
[x][xy]
EOF

test_case 'parameters:*'
test_shell_succeed -s "a b c" d "e  f" << "EOF"
printf '[%s]' $*
printf '\n'
printf '[%s]' "$*"
printf '\n'
printf '[%s]' "x$*y"
printf '\n'
IFS=x
printf '[%s]' "$*"
printf '\n'
IFS=
printf '[%s]' "$*"
printf '\n'
# TODO: Check with one empty parameter expansion
EOF
assert_output << EOF
[a][b][c][d][e][f]
[a b c d e  f]
[xa b c d e  fy]
[a b cxdxe  f]
[a b cde  f]
EOF

test_case 'parameters:#'
test_shell_succeed << "EOF"
echo $#
set -- one two three
echo $#
EOF
assert_output << EOF
0
3
EOF

test_case 'parameters:?'
test_shell_succeed << "EOF"
echo $?
(exit 1)
echo $?
(exit 123)
echo $?
EOF
# TODO: var=$(exit 42)
assert_output << EOF
0
1
123
EOF

test_case 'parameters:-'
for option in a b C e f u v x; do
    case $(test_shell -$option -c 'echo $-' 2>/dev/null) in
    *$option*) ;;
    *) fail_test "\$- does not contain '$option'"
    esac
done

test_case 'parameters:$'
test_shell 2>/dev/null << "EOF"
test $$ = $(echo $$) || exit 1
kill $$
EOF
test $? -gt 128 || fail_test "Shell exited with wrong status"

# TODO: $!

test_case 'parameters:0'
test $(test_shell -c 'echo $0' command_name) = command_name || fail_test '$0 was set incorrectly'

test_case 'parameters:IFS'
test_shell_succeed -c 'printf "%s" "$IFS"'
printf ' \t\n' | assert_output

test_case 'parameters:PPID'
test_shell_succeed -c 'echo $PPID'
echo $$ | assert_output

test_case 'parameters:PWD'
test "$(test_shell -c 'echo "$PWD"')" = "$PWD" || fail_test "Shell did not take PWD from the environment"
# TODO: test_pwd="$(PWD=/dev/null test_shell -c 'echo "$PWD"')"
# test "$(cd "$test_pwd" && ls -di .)" = "$(ls -di .)" || fail_test "Shell did not set PWD correctly"

end_test_set
