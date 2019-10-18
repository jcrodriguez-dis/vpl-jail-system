#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

<<<<<<< HEAD
AC_PREREQ([2.68])
AC_INIT([vpl jail system], [2.5.0])
=======
AC_PREREQ([2.65])
AC_INIT([vpl jail system], [2.4.1.0])
>>>>>>> refs/heads/V2.4.1
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([util], [forkpty],)
AC_CHECK_LIB([crypto], [SHA1],)
AC_CHECK_LIB([ssl], [SSL_CTX_new],)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT64_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
CPPFLAGS="-Wall"
# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([inet_ntoa lchown memset mkdir regcomp rmdir socket strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
