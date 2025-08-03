# DX_FUNC_TCGETWINSIZE
# Check for the tcgetwinsize function and provide it if it is missing.
AC_DEFUN([DX_FUNC_TCGETWINSIZE], [dnl
AC_CHECK_FUNCS([tcgetwinsize], [],
    [AC_LIBOBJ([tcgetwinsize])
    AC_CHECK_HEADERS([sys/ioctl.h])
    AC_CHECK_TYPES([struct winsize], [], [],
[#include <termios.h>
#if HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif])
    ])
])
