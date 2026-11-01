# DX_WITH_SHELL_INT
# Allows to select the type to be used for shell arithmetic.
AC_DEFUN([DX_WITH_SHELL_INT], [dnl
AC_ARG_WITH([shell-int],
    [AS_HELP_STRING([--with-shell-int],
        [choose type used for shell arithmetic @<:@default=signed long@:>@])],
    [],
    [with_shell_int="signed long"])
AS_CASE([$with_shell_int],
    [long | "signed long"],
        [MIN_SHELL_INT=LONG_MIN
        MAX_SHELL_INT=LONG_MAX
        SHELL_INT_PRId=\"ld\"
        parse_shell_int=strtol
        ],
    ["long long" | "signed long long"],
        [MIN_SHELL_INT=LLONG_MIN
        MAX_SHELL_INT=LLONG_MAX
        SHELL_INT_PRId=\"lld\"
        parse_shell_int=strtoll
        ],
    [intmax_t],
        [MIN_SHELL_INT=INTMAX_MIN
        MAX_SHELL_INT=INTMAX_MAX
        SHELL_INT_PRId=\"jd\"
        parse_shell_int=strtoimax
        ],
    [AC_MSG_FAILURE([Cannot use type $with_shell_int for shell arithmetic.
Supported types are: (signed) long, (signed) long long, intmax_t])]
    )
AC_DEFINE_UNQUOTED([ShellInt], [$with_shell_int],
    [Define to type used for shell arithmetic.])
AC_DEFINE_UNQUOTED([MIN_SHELL_INT], [$MIN_SHELL_INT],
    [Minimum value of ShellInt.])
AC_DEFINE_UNQUOTED([MAX_SHELL_INT], [$MAX_SHELL_INT],
    [Maximum value of ShellInt.])
AC_DEFINE_UNQUOTED([SHELL_INT_PRId], [$SHELL_INT_PRId],
    [Printf conversion for formatting ShellInt as decimal.])
AC_DEFINE_UNQUOTED([parse_shell_int], [$parse_shell_int],
    [Define to function with strtol interface for parsing ShellInt.])
])
