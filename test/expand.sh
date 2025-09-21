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

# Tests for POSIX XSH 2.6 Word Expansions.

# TODO: Tilde expansion, arithmetic expansion

test_case 'expand:parameter:simple'
test_shell_succeed << "EOF"
set_var=x
empty_var=
unset -v unset_var
echo $set_var ${empty_var}
echo $set_var${unset_var}x
EOF
assert_output << EOF
x
xx
EOF

test_case 'expand:parameter:default'
test_shell_succeed << "EOF"
set_var=x
empty_var=
unset -v unset_var
echo ${set_var:-$(kill $$)} $set_var
echo ${empty_var:-"$set_var y"} $empty_var
echo ${unset_var:-"$set_var y"} $unset_var
echo ${set_var-$(kill $$)} $set_var
echo ${empty_var-$(kill $$)} $empty_var
echo ${unset_var-"$set_var y"} $unset_var
EOF
assert_output << EOF
x x
x y
x y
x x

x y
EOF

test_case 'expand:parameter:assign'
test_shell_succeed << "EOF"
set_var=x
empty_var=
unset -v unset_var
echo ${set_var:=$(kill $$)} $set_var
echo ${empty_var:="$set_var y"} $empty_var
echo ${unset_var:="$set_var y"} $unset_var
empty_var=
unset -v unset_var
echo ${set_var=$(kill $$)} $set_var
echo ${empty_var=$(kill $$)} $empty_var
echo ${unset_var="$set_var y"} $unset_var
EOF
assert_output << EOF
x x
x y x y
x y x y
x x

x y x y
EOF

test_case 'expand:parameter:error'
test_shell_fail << "EOF"
set_var=x
empty_var=
unset -v unset_var
echo ${set_var:?$(kill $$)} $set_var
(echo ${empty_var:?empty_var is null} $empty_var) && exit 0
(echo ${unset_var:?unset_var is null} $unset_var) && exit 0
empty_var=
unset -v unset_var
echo ${set_var?$(kill $$)} $set_var
echo ${empty_var?$(kill $$)} $empty_var
echo ${unset_var?unset_var is unset} $unset_var
echo fail
EOF
assert_output << EOF
x x
x x

EOF
grep -q "empty_var is null" test_stderr || fail_test '${:?} does not work for empty variable'
grep -q "unset_var is null" test_stderr || fail_test '${:?} does not work for unset variable'
grep -q "unset_var is unset" test_stderr || fail_test '${?} does not work for unset variable'

test_case 'expand:parameter:alternative'
test_shell_succeed << "EOF"
msg=hello
set_var=x
empty_var=
unset -v unset_var
echo ${set_var:+$msg} $set_var
echo ${empty_var:+$(kill $$)} $empty_var
echo ${unset_var:+$(kill $$)} $unset_var
empty_var=
unset -v unset_var
echo ${set_var+$msg} $set_var
echo ${empty_var+$msg} $empty_var
echo ${unset_var+$(kill $$)} $unset_var
EOF
assert_output << EOF
hello x


hello x
hello

EOF

test_case 'expand:parameter:length'
test_shell_succeed << "EOF"
var1=abcdef
var2=
var3=rfvdzuwdwoidfmjweufdnhwe
var4=123456789
unset var5
echo ${#var1} ${#var2} ${#var3} ${#var4} ${#var5}
EOF
assert_output << EOF
6 0 24 9 0
EOF

# TODO: Prefix/suffix removal

test_case 'expand:command:new_style'
test_shell_succeed << "EOF"
echo $(echo Hello)
echo "$(
    echo abc
    echo
    echo "def" # Comments are allowed in command substitutions. )"`
    echo
    )"
echo $(echo $(echo nested))
var=x
echo $(var=y; echo $var) $var
echo "$(case x in x) echo match;; *) echo fail; esac) abc)"
EOF
assert_output << EOF
Hello
abc

def
nested
y x
match abc)
EOF

test_case 'expand:command:old_style'
test_shell_succeed << "EOF"
echo `echo Hello`
echo "`
    echo abc
    echo
    # TODO: Put a double quote in the comment to check that the shell ignores it.
    echo "def" # Comments are allowed in command substitutions. )\`
    echo
    `"
echo `echo \`echo nested\``
var=x
echo `var=y; echo $var` $var
echo "`case x in x) echo match;; *) echo fail; esac` abc)"
EOF
assert_output << EOF
Hello
abc

def
nested
y x
match abc)
EOF

test_case 'expand:field_split'
test_shell_succeed << "EOF"
var="a  b	c
d "
printf '[%s]' $var
printf '\n'
unset IFS
printf '[%s]' $var
printf '\n'
IFS=' '
printf '[%s]' $var
printf '\n'
IFS=x
printf '[%s]' $var
printf '\n'
var=axbxxcdxex
printf '[%s]' axb $var
printf '\n'
var=
printf '[%s]' x $var
printf '\n'
IFS=
printf '[%s]' x $var
printf '\n'
var="a b"
unset IFS
printf '[%s]' ${IFS=' '} $var
printf '\n'
# TODO: Test that multiple non-whitespace IFS character delimit multiple fields
#IFS=x
#var="xxa"
#printf '[%s]' $var
#printf '\n'
EOF
assert_output << EOF
[a][b][c][d]
[a][b][c][d]
[a][b	c
d]
[a  b	c
d ]
[axb][a][b][][cd][e]
[x]
[x]
[a][b]
EOF

end_test_set
