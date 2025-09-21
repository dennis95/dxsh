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

# Tests for POSIX XSH 2.7 Redirection.

# TODO: Test closing fd.

test_case 'redirection:number'
test_shell_succeed << "EOF"
echo 1 > foo
echo \2>>foo
echo "3">>foo
fd=4
echo $fd>>foo
echo 5\>foo
cat foo
EOF
assert_output << EOF
5>foo
1
2
3
4
EOF
rm -f foo

test_case 'redirection:input'
echo Test > foo
test_shell_succeed << "EOF"
file=foo
cat < $file
{ cat <&9; } 9<foo
EOF
assert_output << EOF
Test
Test
EOF

test_case 'redirection:output'
test_shell_succeed << "EOF"
echo Hello >foo
cat foo
{ echo World >&9; } 9> foo
cat foo
echo abc >| foo
cat foo
set -C
(echo overwrite >foo) 2>/dev/null && exit 1
cat foo
echo clobber >| foo
cat foo
EOF
assert_output << EOF
Hello
World
abc
abc
clobber
EOF

test_case 'redirection:append'
test_shell_succeed << "EOF"
echo Hello >foo
echo World >>foo
cat foo
set -C
echo append >> foo
cat foo
EOF
assert_output << EOF
Hello
World
Hello
World
append
EOF
rm -f foo

test_case 'redirection:read_write'
test_shell_succeed << "EOF"
echo Hello 1<> foo
{
    cat
    echo World >&0
} <> foo
cat foo
EOF
assert_output << EOF
Hello
Hello
World
EOF
rm -f foo

test_case 'redirection:here_doc'
# TODO: Test backslash newline
test_shell_succeed << "EOT"
var=x
cat << EOF
Hello "$var"
abc
$(echo EOF)
EOF
: || cat << EOF
$(echo fail >&2)
EOF
{ cat <&9; } 9<< $END
Test $END x
$END
cat << "END"
literal $var
END
cat <<- EOF
			Indented text
	EOF
cat << END1; echo ---; cat << END2
Hello
END1
World
END2
EOT
assert_output << "EOT"
Hello "x"
abc
EOF
Test  x
literal $var
Indented text
Hello
---
World
EOT

end_test_set
