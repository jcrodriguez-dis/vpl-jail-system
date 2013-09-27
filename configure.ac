#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(vpl_xmlrpc_jail, 1.2, jcrodriguez@dis.ulpgc.es)
AM_INIT_AUTOMAKE(vpl_xmlrpc_jail, 1.2, jcrodriguez@dis.ulpgc.es)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([util], [forkpty])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h netinet/in.h stdint.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset mkdir rmdir socket])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
