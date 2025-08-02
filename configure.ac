m4_include([m4/check-cflags.m4])

AC_PREREQ([2.71])
AC_INIT([dxsh], [0.9dev])
AC_CONFIG_SRCDIR([dxsh.c])
AC_CONFIG_HEADERS([config.h])
AC_ARG_PROGRAM

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AC_PROG_INSTALL
AC_CHECK_TOOL([STRIP], [strip], [:])

# Enable warnings on GCC.
DX_CHECK_CFLAGS([-Wall])
DX_CHECK_CFLAGS([-Wextra])
DX_CHECK_CFLAGS([-Werror=implicit-function-declaration])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
