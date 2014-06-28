#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.69])
AC_INIT([transtreamproxy], [3.0.0], [oskwon@dev3])
AM_INIT_AUTOMAKE([transtreamproxy], [3.0.0])

#AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_GNU_SOURCE

# Checks for programs.
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([boost/crc.hpp], [], [AC_MSG_ERROR(No found boost libraries.)])
AC_LANG_POP([C++])

AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(rt, shm_open)

# Checks for libraries.
AC_SUBST(PLATFORM)

# Checks for header files.
AC_SUBST(INCPATH)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

