m4_include([m4/check-cflags.m4])
m4_include([m4/tcgetwinsize.m4])

AC_PREREQ([2.71])
AC_INIT([dxsh], [0.9dev])
AC_CONFIG_SRCDIR([dxsh.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_LIBOBJ_DIR([compat])
AC_ARG_PROGRAM

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AC_PROG_INSTALL
AC_CHECK_TOOL([STRIP], [strip], [:])

DX_FUNC_TCGETWINSIZE
AC_REPLACE_FUNCS([sig2str str2sig])
AS_IF([test "$ac_cv_func_sig2str" = no || test "$ac_cv_func_str2sig" = no ],
    [AC_LIBOBJ(signalnames)])

# Enable warnings on GCC.
DX_CHECK_CFLAGS([-Wall])
DX_CHECK_CFLAGS([-Wextra])
DX_CHECK_CFLAGS([-Werror=implicit-function-declaration])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
