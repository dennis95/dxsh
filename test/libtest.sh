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

tests_failed=false
current_test_failed=false
unset current_test_case

# run_shell [ARGS...]
# Execute the run shell.
run_shell() {
    command "$run_shell" $run_shell_args "$@"
}

# test_shell [ARGS...]
# Execute the test shell.
test_shell() {
    command "$test_shell" $test_shell_args "$@"
}

# test_case NAME
# Start a new test case. Also prints the success message of the previous test.
test_case() {
    end_test
    current_test_case=$1
    current_test_failed=false
}

# fail_test MESSAGE
# Mark the current test case as failed. The test case will still continue
# executing.
fail_test() {
    echo "$current_test_case FAILED: $1"
    current_test_failed=true
    tests_failed=true
}

# test_shell_success [ARGS...]
# Run the test shell and assert that it succeeds and produces no output to
# stderr. Output is captured in the test_stdout file.
test_shell_succeed() {
    test_shell "$@" >test_stdout 2>test_stderr || fail_test "Shell returned error status"
    test -s test_stderr && fail_test "Shell wrote to standard error:
$(cat test_stderr)"
}

# test_shell_fail [ARGS...]
# Run the test shell and assert that it returns a non-zero status. The exit
# status is stored in the exit_status variable. Output is captured in the
# test_stdout and test_stderr files.
test_shell_fail() {
    test_shell "$@" >test_stdout 2>test_stderr
    exit_status=$?
    test $exit_status -ne 0 || fail_test "Shell returned success status"
}

# assert_output
# Assert that the captured output matches the input of this command.
assert_output() {
    diff -u - test_stdout || fail_test "output differed"
}

# assert_stderr
# Assert that the captured stderr matches the input of this command.
assert_stderr() {
    diff -u - test_stderr || fail_test "stderr differed"
}

# end_test
# Prints the success message for the current test case. Should not be called
# directly.
end_test() {
    if test -n "$current_test_case" && ! $current_test_failed; then
        echo "$current_test_case PASSED"
    fi
}

# end_test_set
# Should be called at the end of a test set.
end_test_set() {
    end_test
    if $tests_failed; then
        exit 1
    fi
}
