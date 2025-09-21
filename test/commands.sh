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

# Tests for POSIX XSH 2.9 Shell Commands.

test_case 'commands:simple:without_name'
test_shell_succeed << "EOF"
empty=
a=x b=y >foo $empty
echo $a $b
$(cat foo) >foo
>foo c=z
echo $c
EOF
assert_output << EOF
x y
z
EOF
rm -f foo

test_case 'commands:simple:assignments'
echo 'echo =foo $*' >=foo
echo 'echo prog $a $b $c' >prog
chmod +x =foo prog
test_shell_succeed << "EOF"
export PATH="$(pwd):$PATH"
unset a b c
a=b =foo x=bar
cmd=prog
a=xyz
a=${c=$b}x c=${c}y ${b=$cmd} >$a
cat xyz
echo $a $b $c
empty=
c=ccc $empty $cmd
EOF
assert_output << EOF
=foo x=bar
prog progx progy
xyz prog prog
prog ccc
EOF
rm -f =foo prog xyz

test_case 'commands:simple:search'
mkdir foo bar
echo 'echo foo $*' > foo/prog
echo 'echo bar $*' > bar/prog
chmod +x foo/prog bar/prog
test_shell_succeed << "EOF"
./foo/prog abc
bar/prog def
export PATH="$(pwd)/foo:$(pwd)/bar:$PATH"
chmod -x foo/prog
prog only bar/prog is executable
chmod +x foo/prog
PATH="$PATH" # Prevent test shell from caching the location.
prog now foo/prog is executable
prog() {
    echo function $*
}
prog now a function is called
EOF
assert_output << EOF
foo abc
bar def
bar only bar/prog is executable
foo now foo/prog is executable
function now a function is called
EOF
rm -rf foo bar

test_case 'commands:pipeline'
test_shell_succeed << "EOF"
echo World | { echo Hello; cat; }
echo abc | echo def | echo ghi
! (exit 42)
echo $?
! (exit 0)
echo $?
echo abc | (cat; exit 42)
echo $?
! echo def | { cat; false; }
echo $?
EOF
assert_output << EOF
Hello
World
ghi
0
1
abc
42
def
0
EOF

test_case 'commands:list:and_or'
test_shell_succeed << "EOF"
true && echo Hello
false && echo World
echo abc && echo def && echo ghi
false && echo x && echo y
true || echo fail
false || echo xyz
echo abc || echo def || echo ghi
false && echo t || echo f
true && echo t || echo f
false || echo f && echo t
true || echo f && echo t
EOF
assert_output << EOF
Hello
abc
def
ghi
xyz
abc
f
t
f
t
t
EOF

# TODO: asynchronous lists

test_case 'commands:list:sequential'
test_shell_succeed << "EOF"
echo abc; echo def
true || echo x; echo y
false && echo a; echo b
EOF
assert_output << EOF
abc
def
y
b
EOF

test_case 'commands:compound:subshell'
test_shell_succeed << "EOF"
var=x
(
    echo Hello
    var=y
    echo $var
)
echo $var
(true) && echo success || echo fail
(false) && echo success || echo fail
(echo abc >&2) 2>&1
EOF
assert_output << EOF
Hello
y
x
success
fail
abc
EOF

test_case 'commands:compound:brace_group'
test_shell_succeed << "EOF"
var=x
{
    echo Hello
    var=y
    echo $var
}
echo $var
{ true;} && echo success || echo fail
{ false;} && echo success || echo fail
{ echo abc >&2; } 2>&1
EOF
assert_output << EOF
Hello
y
y
success
fail
abc
EOF

test_case 'commands:compound:for'
test_shell_succeed -s one two three four "five six" << "EOF"
for x in a b c d
do
    echo $x
done
echo $x
for number
do
    echo $number
done
EOF
assert_output << EOF
a
b
c
d
d
one
two
three
four
five six
EOF

test_case 'commands:compound:case'
test_shell_succeed << "EOF"
pattern="a b"
case "a b" in
    ($pattern) echo match
esac
case x in
    y) echo fail;;
    a|x) echo x;;
    *) echo fail;;
esac
EOF
assert_output << EOF
match
x
EOF

test_case 'commands:compound:case_fallthrough'
test_shell_succeed << "EOF"
pattern="a b"
case $pattern in
    a*) echo a ;&
    b) echo b ;&
    *) echo all
esac
EOF
assert_output << EOF
a
b
all
EOF

test_case 'commands:compound:if'
test_shell_succeed << "EOF"
if true; then
    echo true
fi
if false
then echo false
else
    echo else
fi

if false; then
    echo a
elif true; then
    echo b
else echo c
fi

if false; then
    echo d
elif false; then
    echo e
elif true; then
    echo f
fi
EOF
assert_output << EOF
true
else
b
f
EOF

test_case 'commands:compound:while'
test_shell_succeed << "EOF"
var=
while test "$var" != xxxxx
do
    var=${var}x
    echo $var
done
while false; do :; done
echo $?
EOF
assert_output << EOF
x
xx
xxx
xxxx
xxxxx
0
EOF

test_case 'commands:compound:until'
test_shell_succeed << "EOF"
var=
until test "$var" = xxxxx
do
    var=${var}x
    echo $var
done
until true; do :; done
echo $?
EOF
assert_output << EOF
x
xx
xxx
xxxx
xxxxx
0
EOF

test_case 'commands:function'
test_shell_succeed << "EOF"
set -- one two three
echo $@
func() {
    echo function $@
}
func a b c
echo $@

func () if test $1 = x
then echo Hello $@
else echo function $@
fi

func x y z
func a b c

echo foo > foo
func() {
    echo function $@
} > foo
cat foo
func redirected
cat foo
func again
cat foo
EOF
assert_output << EOF
one two three
function a b c
one two three
Hello x y z
function a b c
foo
function redirected
function again
EOF
rm -f foo

end_test_set
