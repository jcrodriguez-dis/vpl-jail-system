#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([vpl jail system], [4.0.3], jc.rodriguezdelpino@ulpgc.es)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_LANG_PUSH(C++)
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

# Checks for libraries.
AC_CHECK_LIB([util], [forkpty],)
AC_CHECK_LIB([crypto], [SHA1],)
AC_CHECK_LIB([ssl], [SSL_CTX_new],)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h syslog.h unistd.h])
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <filesystem>], [auto p = std::filesystem::weakly_canonical("dummy")])],
               [AC_DEFINE([HAVE_WEAKLY_CANONICAL], [1],
                          [Define to 1 if you have the weakly_canonical function using the filesystem header file])],
               [AC_MSG_NOTICE([The weakly_canonical function in the filesystem library is not available])])
AM_CONDITIONAL([CPP17], [test ! x"$HAVE_WEAKLY_CANONICAL" = x1])
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
AM_INIT_AUTOMAKE

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([getcwd inet_ntoa lchown memset mkdir regcomp rmdir socket\
                strerror TLS_server_method SSL_CTX_set_ciphersuites])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

