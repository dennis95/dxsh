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

# Tests for POSIX XSH 2.15 Special Built-In Utilities and XSH 3. Utilities.

# assert_special_builtin NAME [INVOKATION]
# Assert that NAME is a special builtin. The given INVOKATION is used to call
# the builtin. If INVOKATION is not given, it defaults to NAME. The invokation
# is guaranteed to happen in a function and loop context. The invokation should
# not cause the shell to exit (except for the exit builtin).
assert_special_builtin() {
    invokation="${2-$1}"
    test_shell << EOF || fail_test "Variable assignments do not affect current shell environment for special builtin $1."
export PATH=/dev/null
is_special_builtin=no
fn() {
    execute_loop=:
    while eval "\$execute_loop"; do
        execute_loop="! :"
        is_special_builtin=yes $invokation </dev/null
    done
}
fn ignored
case "\$is_special_builtin" in
yes) exit 0;;
*) exit 1
esac
EOF
}

# assert_intrinsic NAME [INVOKATION]
# Assert that NAME is an intrinsic utility. The given INVOKATION is used to call
# the builtin. If INVOKATION is not given, it defaults to NAME. The invokation
# is guaranteed to happen in a function and loop context. The invokation should
# not cause the shell to exit.
assert_intrinsic() {
    invokation="${2-$1}"
    test_shell << EOF || fail_test "Functions do not take precedence over intrinsic utility $1."
export PATH=/dev/null
function_called=no
$1() {
    function_called=yes
}
fn() {
    execute_loop=:
    while eval "\$execute_loop"; do
        execute_loop="! :"
        $invokation </dev/null
    done
}
fn ignored
case "\$function_called" in
yes) exit 0;;
*) exit 1
esac
EOF
    test_shell << EOF || fail_test "Variable assignments do affect current shell environment for intrinsic utility $1."
export PATH=/dev/null
is_special_builtin=no
fn() {
    execute_loop=:
    while eval "\$execute_loop"; do
        execute_loop="! :"
        is_special_builtin=yes $invokation </dev/null
    done
}
fn ignored
case "\$is_special_builtin" in
no) exit 0;;
*) exit 1
esac
EOF
}

test_case 'builtins:special:break'
assert_special_builtin break
test_shell_succeed << "EOF"
while true; do
    echo while loop
    break
done
echo $?
until false; do
    echo until loop
    break
done
echo $?
for x in a b c; do
    echo for loop $x
    break
done
echo $?
for x in one two three; do
    while true; do
        until false; do
            echo $x
            break 2
        done
    done
done
while true; do
    while true; do
        break 999
    done
done
echo $?
while true; do
    while break 2; do
        echo fail
    done
done
echo $?
EOF
assert_output << EOF
while loop
0
until loop
0
for loop a
0
one
two
three
0
0
EOF

test_case 'builtins:special:colon'
assert_special_builtin :
test_shell_succeed << "EOF"
: && echo success
: || echo fail
: --@ hfrnegvjmr && echo : ignores arguments
EOF
assert_output << EOF
success
: ignores arguments
EOF

test_case 'builtins:special:continue'
assert_special_builtin continue
test_shell_succeed << "EOF"
var=
while test "$var" != xxx; do
    var=${var}x
    if test "$var" = xx; then
        continue
    fi
    echo $var
done
var=
until test "$var" = yyy; do
    var=${var}y
    if test "$var" = yy; then
        continue
    fi
    echo $var
done
for x in a b c; do
    if test $x = b; then continue; fi
    echo $x
done
for x in one two three; do
    while true; do
        until false; do
            echo $x
            continue 3
        done
    done
done
for x in a b c; do
    while true; do
        echo $x
        continue 999
    done
done
for x in a b c; do
    while continue 2; do
        echo fail
    done
done
EOF
assert_output << EOF
x
xxx
y
yyy
a
c
one
two
three
a
b
c
EOF

test_case 'builtins:special:dot'
assert_special_builtin . ". /dev/null"
cat > foo << "EOF"
foo() {
    echo foo called: $@
}
echo foo loaded
EOF
cat > bar << "EOF"
bar() {
    echo bar called: $@
}
var="Variable set by bar script"
EOF
test_shell_succeed << "EOF"
. -- ./foo
foo Hello
var=xyz
export PATH="$(pwd):$PATH"
. bar
bar $var
EOF
assert_output << EOF
foo loaded
foo called: Hello
bar called: Variable set by bar script
EOF
rm -f foo bar

test_case 'builtins:special:eval'
assert_special_builtin eval "eval :"
test_shell_succeed << "EOF"
eval || echo fail
empty=
eval $empty || echo empty fail
var=foo
eval $var=World
eval echo Hello $foo
defun='() {' endfunc=\;}
eval my_eval $defun eval '$@' $endfunc
x=\$y
y=42
my_eval echo $x
EOF
assert_output << EOF
Hello World
42
EOF

test_case 'builtins:special:exec'
assert_special_builtin exec
test_shell_succeed << "EOF"
exec
exec 9>&1 >foo
echo Hello
cat foo >&9
exec -- >&9
cat foo
exec echo Hello exec
This is unreachable.
EOF
assert_output << EOF
Hello
Hello
Hello exec
EOF
rm -f foo

test_case 'builtins:special:exit'
assert_special_builtin exit "exit 0"
test_shell -c 'exit' || fail_test "exit in new shell does not succeed"
test_shell -c 'exit 42'; exit_status=$?
test $exit_status = 42 || fail_test "exit 42 exited with $exit_status"
test_shell -c 'false; exit' && fail_test "exit did not return status of previous command"
test_shell_succeed << "EOF"
echo Test
exit
echo failed
EOF
assert_output << EOF
Test
EOF

test_case 'builtins:special:export'
assert_special_builtin export "export a=b"
test_shell_succeed << "EOF"
export env_var="some test value"
echo $env_var
# TODO: Test restoring env vars
#reset_env_var=$(export -p | grep '^export env_var=')
unset env_var
echo $env_var
export env_var
#export -p | grep '^export env_var$'
#eval "$reset_env_var"
#echo $env_var
unset a b c
export a=1 b c=2
echo $a $b $c
EOF
assert_output << EOF
some test value

1 2
EOF

# TODO: readonly

test_case 'builtins:special:return'
assert_special_builtin return
cat > foo << "EOF"
if test "$var" = 42; then
    return 42
fi
echo $var
EOF
test_shell_succeed << "EOF"
set_status() {
    return $1
    echo unreachable
}
set_status 42
echo $?
set_status 255
echo $?
set_status 0
echo $?
var=xyz
. ./foo
echo $?
var=42
. ./foo
echo $?
# TODO: Check that return works within eval
#func() {
#    eval "return 0"
#    exit 1
#}
#func
EOF
assert_output << EOF
42
255
0
xyz
0
42
EOF
rm -f foo

test_case 'builtins:special:set'
assert_special_builtin set "set --"
test_shell_succeed << "EOF"
foo=x
# TODO: Test output of set
#reset_foo=$(set | grep '^foo=')
#foo=wrong
#eval "$reset_foo"
echo $foo
set one two three
echo $@
set -- -o abc
echo $@

# TODO: Test shell options
EOF
assert_output << EOF
x
one two three
-o abc
EOF

test_case 'builtins:special:shift'
assert_special_builtin shift
test_shell_succeed -s one two three four five << "EOF"
echo $# $@
shift
echo $# $@
shift 2
echo $# $@
shift 0
echo $# $@
shift 2
echo $# $@
EOF
assert_output << EOF
5 one two three four five
4 two three four five
2 four five
2 four five
0
EOF

# TODO: times

test_case 'builtins:special:trap'
assert_special_builtin trap "trap - USR1"
test_shell_succeed << "EOF"
trap 'echo USR1 received' USR1
echo send USR1
kill -s USR1 $$
# TODO: Test restoring traps.
#set_usr1="$(trap -p)"
# TODO: Test setting traps to ignore.
#trap '' USR1
#echo send ignored USR1
#kill -s USR1 $$
#eval "$set_usr1"
#echo send restored USR1
#kill -s USR1 $$
unset var
trap 'echo in trap $?; var=trapped; false' USR1
kill -s USR1 $$
echo $var $?
trap 'echo exit trap $?' EXIT
(exit 1)
echo will now exit
EOF
assert_output << EOF
send USR1
USR1 received
in trap 0
trapped 0
will now exit
exit trap 0
EOF

test_case 'builtins:special:unset'
assert_special_builtin unset "unset unset_var"
cat > foo << "EOF"
echo script called ${foo-unset}
EOF
chmod +x foo
test_shell_succeed << "EOF"
export PATH="$(pwd):$PATH"
export foo=xyz
foo() {
    echo function called ${foo-unset}
}
foo
unset foo
foo
foo=Hello
unset -f foo
foo
export foo
foo
foo() {
    echo function called
}
unset -fv foo
foo
EOF
assert_output << EOF
function called xyz
function called unset
script called unset
script called Hello
script called unset
EOF
rm -f foo

test_case 'builtins:intrinsic:cd'
assert_intrinsic cd "cd ."
mkdir foo bar baz
echo foo > foo/file
echo bar > bar/file
echo baz > baz/file
echo parent > file
test_shell_succeed << "EOF"
parent_dir=$PWD
cd foo
cat file
cd ..
cat file
export HOME="$parent_dir/bar"
cd
cat file
cd $parent_dir/baz
cat file
# TODO: -eLP options, CDPATH, OLDPWD
EOF
assert_output << EOF
foo
parent
bar
baz
EOF
rm -rf foo bar baz

test_case 'builtins:intrinsic:command'
assert_intrinsic command "command :"
echo 'echo script $@' > foo
chmod +x foo
test_shell_succeed << "EOF"
export PATH="$PWD:$PATH"
foo() {
    echo function $@
}
command foo abc def
command echo Hello
command -v foo
EOF
assert_output << EOF
script abc def
Hello
foo
EOF
rm -f foo

test_case 'builtins:intrinsic:read'
assert_intrinsic read "read field"
test_shell_succeed << "EOF"
printf '%s\n' "abc  def  ghi " "jkl mno pqr " > file
{
    read field1 field2
    read field3 field4 field5 field6
} <file
printf '[%s]' "$field1" "$field2" "$field3" "$field4" "$field5" "$field6"
printf '\n'
printf '%s\n' 'a\\b c\ d \  e' >file
read field1 field2 field3 field4 field5 <file
printf '[%s]' "$field1" "$field2" "$field3" "$field4" "$field5"
printf '\n'
printf '%s\n' 'a\\b c\ d \  e' >file
read -r field1 field2 field3 field4 field5 <file
printf '[%s]' "$field1" "$field2" "$field3" "$field4" "$field5"
printf '\n'
echo abcxdef >file
read -d x field1 <file
echo "$field1"
printf '%s\n' "abc def" >file
IFS= read field1 field2 < file
printf '[%s]' "$field1" "$field2"
printf '\n'
EOF
assert_output << "EOF"
[abc][def  ghi][jkl][mno][pqr][]
[a\b][c d][ ][e][]
[a\\b][c\][d][\][e]
abc
[abc def][]
EOF
rm -f file

test_case 'builtins:intrinsic:umask'
assert_intrinsic umask "umask >/dev/null"
test_shell_succeed << "EOF"
# TODO: -S option, symbolic mask operand
saved_mask=$(umask)
umask 777
: >file
cat file 2>/dev/null && echo "we should be able to read this"
rm -f file
umask $saved_mask
echo Hello > file
cat file
EOF
assert_output << EOF
Hello
EOF

end_test_set
