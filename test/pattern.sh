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

# Tests for POSIX XSH 2.14 Pattern Matching Notation.

test_case 'pattern:case:normal'
# assert_match PATTERN STRING
# Assert that PATTERN matches STRING. None of the characaters in pattern will be
# quoted.
assert_match() {
    test_shell -s "$1" "$2" << "EOF" || fail_test "Pattern '$1' did not match string '$2'"
case $2 in
$1) exit 0 ;;
*) exit 1
esac
EOF
}

# assert_no_match PATTERN STRING
# Assert that PATTERN does not match STRING. None of the characaters in pattern
# will be quoted.
assert_no_match() {
    test_shell -s "$1" "$2" << "EOF" || fail_test "Pattern '$1' did match string '$2'"
case $2 in
$1) exit 1 ;;
*) exit 0
esac
EOF
}

assert_no_match '' 'x'
assert_match 'a*c' 'abc'
assert_match '*x*y*z' 'abcxydez'
assert_no_match '*x*y*z' 'abcxydezf'
assert_no_match '??' 'x'
assert_no_match 'a' 'A'
assert_match 'a?c' 'abc'
assert_no_match 'a?c' 'abdc'
assert_no_match 'a?' 'a'
assert_match 'a[bc]d' 'abd'
assert_match 'a[bc]d' 'acd'
assert_match '[a-c]' 'b'
assert_no_match '[!a-c]' 'b'
assert_match '[!a-c]' 'd'
assert_match '[?]' '?'
assert_no_match '[?]' 'x'
assert_match '[]-_]' '^'
assert_match '[!]]' 'x'
assert_no_match '[!]]' ']'
# TODO: Fails due to fnmatch bugs: assert_match '[/\]' '/'
# TODO: Fails due to fnmatch bugs: assert_match '[/\]' '\'
assert_match '[[x]' '['
assert_match '[[:upper:]][[:punct:]]' 'A.'
assert_no_match '[[:upper:]][[:punct:]]' 'a.'
assert_match '[![:upper:]]' 'b'
assert_match '[[:blank:]]' '	'
# TODO: Fails due to fnmatch bugs: assert_match '[\[:alpha:]]' 'b'
assert_match '*a' '.a'
assert_match 'a*c' 'a/c'
assert_match 'a[b/c]d' 'a/d'
assert_match 'a/*b' 'a/.b'
assert_match '*/' 'a/b/'
assert_match 'a\/b' 'a/b'
assert_no_match 'a\*c' 'abc'
assert_match 'a\*c' 'a*c'
assert_match 'a\\c' 'a\c'
assert_match '[x' '[x'
assert_match '[[.x.]]' 'x'
assert_no_match '[[.x.]]' 'y'
assert_match '[![.x.]]' 'a'
assert_match '[[.x.]-[.z.]]' 'y'
assert_no_match '[[.x.]-[.z.]]' 'w'
assert_match '[[=a=]]' 'a'
assert_no_match '[[=a=]]' 'b'
assert_match '[[.[.]]' '['
assert_match '[[.].]]' ']'
assert_match '[[.!.]]' '!'
assert_match '[[.^.]]' '^'
assert_match '[[.-.]]' '-'
assert_match '[[.!.][.^.]]' '^'
assert_no_match '[[.!.][.^.]]' '!^'
assert_no_match '[[.xyz.]]' 'x'
assert_match '[[...]]' '.'
# TODO: Fails due to fnmatch bugs: assert_match '[![:x]:]]' 'x'
# These patterns seem to currently break dxsh.
# TODO: assert_match '[[.\.]]' '\'
# TODO: assert_match '[[:]' '[:'
# TODO: assert_no_match '[[:]' ':'
# TODO: assert_no_match '[[:]' '[[:]'

test_case 'pattern:case:quoted'
# assert_match_quote PATTERN STRING
# Assert that PATTERN matches STRING. PATTERN will undergo word expansion again
# and may thus contain quoting characters.
assert_match_quote() {
    test_shell -s "$2" << EOF || fail_test "Pattern '$1' did not match string '$2'"
case \$1 in
$1) exit 0 ;;
*) exit 1
esac
EOF
}

# assert_no_match_quote PATTERN STRING
# Assert that PATTERN does not match STRING. PATTERN will undergo word expansion
# again and may thus contain quoting characters.
assert_no_match_quote() {
    test_shell -s "$2" << EOF || fail_test "Pattern '$1' did match string '$2'"
case \$1 in
$1) exit 1 ;;
*) exit 0
esac
EOF
}

assert_no_match_quote '""' 'x'
assert_no_match_quote '"a*c"' 'abc'
assert_match_quote '"abc"' 'abc'
assert_match_quote '\abc' 'abc'
assert_no_match_quote 'a\?c' 'abc'
assert_match_quote 'a[bc]d' 'abd'
assert_no_match_quote '"a[bc]d"' 'abd'
assert_match_quote 'a[b\c]d' 'abd'
assert_match_quote 'a[!b\c]d' 'a\d'
assert_match_quote 'a[!b"c"]d' 'a"d'
assert_match_quote 'a["!"bc]d' 'abd'
assert_match_quote 'a["!"bc]d' 'a!d'
assert_no_match_quote '[a"-"c]' 'b'
assert_match_quote '[/\]]' '/'
assert_no_match_quote '[/\]' '\'
assert_match_quote 'a\/b' 'a/b'
assert_match_quote '"a\/b"' 'a\/b'
assert_match_quote '"a\c"' 'a\c'
assert_match_quote '"a\\c"' 'a\c'
assert_match_quote '[["."x.]]' '.]'
# These patterns seem to currently break dxsh.
# TODO: assert_match_quote '[[.x"."]]' '.]'
# TODO: assert_match_quote '[[.x."]"]' '.]'

test_case 'pattern:pathname_expansion'
# check_file_expansion PATTERN FILES...
# Check that PATTERN matches or does not match the given FILES. For each
# filename that begins with ! the ! is removed and the file is expected to not
# match. The  matching filenames must be in alphabetic order. There must be at
# least one match. All paths must be relative.
check_file_expansion() {
mkdir files
(
    shift
    for file; do
        mkdir -p "$(dirname "files/$(echo "$file" | sed 's/^!//')")"
        :> "files/$(echo "$file" | sed 's/^!//')"
    done
) 2>/dev/null

test_shell_succeed << EOF
cd files
echo $1
EOF
shift
sep=
{
    for file; do
        case $file in
        !*) ;;
        *)
            printf "$sep%s" "$file"
            sep=' '
        esac
    done
    echo
} >expected
assert_output <expected
rm -rf files expected
}

check_file_expansion 'a*c' 'abc' 'axc' '!abcd' '!acb'
check_file_expansion '[[:punct:]]' ',' ':' '!.'
check_file_expansion '*a' 'aa' 'ba' '!.a' '!.b'
check_file_expansion '.[ab]' '.a' '.b' '!ab'
check_file_expansion 'a*c' 'abc' '!a/c'
check_file_expansion 'a[b/c]d' 'a[b/c]d' '!abd' '!a/d'
check_file_expansion '*/?' 'a/b' '!c/d/e' 'c/d'
check_file_expansion 'a\/b/*' 'a/b/c'
check_file_expansion 'a\*c/*' 'a*c/x' '!abc/x'

end_test_set
