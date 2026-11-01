# Copyright (c) 2025, 2026 Dennis Wölfing
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

test_case 'expand:tilde:simple'
test_shell_succeed << "EOF"
export HOME="/path/to  /home"
echo ~/~
echo ~/foo
var=~:~/foo
echo "$var"
EOF
assert_output << EOF
/path/to  /home/~
/path/to  /home/foo
/path/to  /home:/path/to  /home/foo
EOF

test_case 'expand:tilde:username'
root_home=$(echo ~root)
case "$root_home" in
/*)
    test_shell_succeed << "EOF"
echo ~root
echo ~root/foo
var=~root:~root/foo
echo "$var"
EOF
    assert_output << EOF
$root_home
$root_home/foo
$root_home:$root_home/foo
EOF
    ;;
*) skip_test "Unable to determine home directory of root";;
esac

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

test_case 'expand:parameter:prefix_suffix'
test_shell_succeed << "EOF"
var=abababa
echo ${var%ba}
echo ${var%%ba}
echo ${var#ab}
echo ${var##ab}
echo ${var%x}
EOF
assert_output << EOF
ababa
ababa
ababa
ababa
abababa
EOF
# Non-trivial patterns are tested in pattern.sh

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

test_case 'expand:arithmetic:constants'
test_shell_succeed << "EOF"
echo $((1)) $(( 123 )) $((2147483647))
echo $(( 0)) $((077)) $((017777777777))
echo $((0x0)) $((0XFF)) $((0x7FFFFFFF ))
EOF
assert_output << EOF
1 123 2147483647
0 63 2147483647
0 255 2147483647
EOF

test_case 'expand:arithmetic:variables'
test_shell_succeed << "EOF"
a=123 b=0x123 c=+2147483647 d=-2147483648
echo $((a)) $((b)) $((c)) $((d))
echo $(($a)) $(($b)) $(($c)) $(($d))
EOF
assert_output << EOF
123 291 2147483647 -2147483648
123 291 2147483647 -2147483648
EOF

test_case 'expand:arithmetic:unary'
test_shell_succeed << "EOF"
x=42
y=-123
echo $((+5)) $((-5)) $((+x)) $((-x)) $((+y)) $((-y))
echo $((+ + 5)) $((+-5)) $((- - -x))
echo $((!1)) $((!0)) $((!x)) $((!y))
echo $((!!100))
echo $((~0)) $((~x)) $((~~y))
echo $((+-~!1))
EOF
assert_output << EOF
5 -5 42 -42 -123 123
5 -5 -42
0 1 0 0
1
-1 -43 -123
1
EOF

test_case 'expand:arithmetic:mult_add_shift'
test_shell_succeed << "EOF"
echo $((+123 * -456))
echo $((12 / 2)) $((13 / 2))
echo $((12 % 2)) $((13 % 2))
echo $((123 / 5 * 5 + 123 % 5))
echo $((10 + 5 * 2 - 1 + -3))
echo $((1 << 30))
echo $((8455289 >> 5))
echo $((2 * 3 + 5 << 2 + 1 >> 1))
EOF
assert_output << EOF
-56088
6 6
0 1
123
16
1073741824
264227
44
EOF

test_case 'expand:arithmetic:comparisons'
test_shell_succeed << "EOF"
echo $((1 < 2)) $((1 > 2)) $((1 <= 2)) $((1 >= 2)) $((1 == 2)) $((1 != 2))
echo $((2 < 2)) $((2 > 2)) $((2 <= 2)) $((2 >= 2)) $((2 == 2)) $((2 != 2))
echo $((3 < 2)) $((3 > 2)) $((3 <= 2)) $((3 >= 2)) $((3 == 2)) $((3 != 2))
echo $(( 1 + 2 == 4 - 1 ))
echo $((5 < 2 < 1)) $((-2 < -1 <= 0))
echo $((1 < 2 == 2 < 3)) $((1 > 2 == 1))
EOF
assert_output << EOF
1 0 1 0 0 1
0 0 1 1 1 0
0 1 0 1 0 1
1
1 0
1 0
EOF

test_case 'expand:arithmetic:bitwise'
test_shell_succeed << "EOF"
echo $((0x3635AA9C & 0x559936CA))
echo $((0x3635AA9C ^ 0x559936CA))
echo $((0x3635AA9C | 0x559936CA))
echo $((3 & 2 ^ 7 | 8))
EOF
assert_output << EOF
336667272
1672256598
2008923870
13
EOF

test_case 'expand:arithmetic:logical'
test_shell_succeed << "EOF"
echo $((0 && 0)) $((0 && 1)) $((1 && 0)) $((1 && 1))
echo $((0 || 0)) $((0 || 1)) $((1 || 0)) $((1 || 1))
echo $((5 && 100)) $((-7 || 3)) $((0 && 123))
echo $((1 && 2 && 3)) $((4 && 5 && 0))
echo $((6 || 7 || 8)) $((9 || 0 || 10))
echo $((1 || 0 && 0))
unset fail
echo $((0 && (fail = 1) ))
test -z "$fail" || exit 1
echo $((1 || (fail = 1) ))
test -z "$fail" || exit 1
EOF
assert_output << EOF
0 0 0 1
0 1 1 1
1 1 0
1 0
1 1
1
0
1
EOF

test_case 'expand:arithmetic:conditional'
test_shell_succeed << "EOF"
result=0
echo $((0 ? 123 : 456)) $((1 ? 123 : 456))
echo $((123 ? 0 : (result = 1) ))
echo $((0 ? result = 2 : 100))
echo $((1 ? 0 ? result = 3 : 1 : (result = 4) ? result = 5 : (result = 6) ))
exit "$result"
EOF
assert_output << EOF
456 123
0
100
1
EOF

test_case 'expand:arithmetic:assignment'
test_shell_succeed << "EOF"
echo $((x = 1))
echo $x
echo $((a = b = 123))
echo $a $b
echo $((a += 3))
echo $((b -= 10))
x=+100
echo $((x *= 2))
echo $((a /= 2))
echo $((b %= 7))
echo $((b <<= 5))
echo $((a >>= 1))
echo $((b |= a))
echo $((b ^= 71))
echo $((a &= b))
echo $a $b $x
: $(( (x += 1) && (x *= 2) ))
echo $x
: $(( (x = 11) - 11 || (x /= 2) ))
echo $x
: $(( (x *= 2) ? x += 1 : (x -= 3) ))
echo $x
echo $((x += x))
echo $(( (x) = 1))
echo $(( (((x))) = 3 ))
echo $x
EOF
assert_output << EOF
1
1
123
123 123
126
113
200
63
1
32
31
63
120
24
24 120 200
402
5
11
22
1
3
3
EOF

value_in_range() {
    # Check that the value is consistent instead of checking for error, some
    # shells don't produce an error but silently overflow the value.
    test "$(test_shell -c 'x='"$1"'; echo $((x))' 2>/dev/null || echo x)" = "$1"
}
unset min_value max_value neg_overflow pos_overflow
if value_in_range -9223372036854775808 && ! value_in_range -9223372036854775809
then
    min_value=-9223372036854775808
    neg_overflow=-9223372036854775809
elif value_in_range -2147483648 && ! value_in_range -2147483649
then
    min_value=-2147483648
    neg_overflow=-2147483649
fi
if value_in_range 9223372036854775807 && ! value_in_range 9223372036854775808
then
    max_value=9223372036854775807
    pos_overflow=9223372036854775808
elif value_in_range 2147483647 && ! value_in_range 2147483648
then
    max_value=2147483647
    pos_overflow=2147483648
fi

test_case 'expand:arithmetic:min_value'
if test -n "$min_value"; then
    # Writing the lowest possible value as a constant is normally undefined
    # because it would be interpreted as a unary minus applied to a positive
    # out-of-range constant. However POSIX requires that $((x)) and $(($x))
    # result in the same value, so test that this works for the minimum.
    test_shell_succeed << EOF
MIN=$min_value
echo \$((MIN))
echo \$((\$MIN))
EOF
    assert_output << EOF
$min_value
$min_value
EOF
else
    skip_test "Unknown minimum value for shell arithmetic"
fi

test_case 'expand:arithmetic:overflow_constant'
if test -n "$pos_overflow"; then
    # An unrepresentable constant is a constraint violation, so there should be
    # an error. Only test this for positive values, since negative values would
    # apply a unary minus to a constant which is undefined.
    test_shell_fail -c ': $(('$pos_overflow'))'
else
    skip_test "Unknown positive overflow value"
fi

test_case 'expand:arithmetic:other_expansions'
test_shell_succeed << "EOF"
echo $(( "1 + 1" ))
op=+
echo $((123 $op 456))
echo $(( $(echo 3) + 4 ""))
x="(4 + 5)"
echo $(("$x" * 2))
open_par='(' closing_par=')'
echo $(( $open_par 1 + 2 $closing_par * 3))
EOF
assert_output << EOF
2
579
7
18
9
EOF

test_case 'expand:field_split'
test_shell_succeed << "EOF"
var=" a  b	c
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
printf '[%s]' $var
printf '\n'
var="a b"
unset IFS
printf '[%s]' ${IFS=' '} $var
printf '\n'
IFS=x
var="xxax"
printf '[%s]' $var
printf '\n'
printf '[%s]' ${var}xb
printf '\n'
var="xxa"
printf '[%s]' ${var}xb
printf '\n'
IFS='x '
var="  xx  a xb"
printf '[%s]' $var
printf '\n'
EOF
assert_output << EOF
[a][b][c][d]
[a][b][c][d]
[a][b	c
d]
[ a  b	c
d ]
[axb][a][b][][cd][e]
[x]
[x]
[a b]
[a][b]
[][][a]
[][][a][xb]
[][][axb]
[][][a][b]
EOF

end_test_set
