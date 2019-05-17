#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ee242_prj2], [0.1], [tomasz.chadzynski@sjsu.edu])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [atan2])

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h stdint.h unistd.h fcntl.h getopt.h errno.h string.h syslog.h signal.h sys/mman.h sys/stat.h inttypes.h limits.h stddef.h string.h sys/time.h sys/timeb.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftime gettimeofday memset pow])


AM_INIT_AUTOMAKE
AC_OUTPUT(Makefile src/Makefile scripts/Makefile)

AC_OUTPUT
