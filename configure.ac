#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([PayZ], [0.1A], [zmnscpxj@protonmail.com])
AC_CONFIG_AUX_DIR([auxdir])
AM_INIT_AUTOMAKE([subdir-objects tar-ustar])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
LT_INIT([disable-shared])

# Checks for programs.
AC_PROG_CC

AX_VALGRIND_CHECK()
AM_CONDITIONAL([USE_VALGRIND], [test x"$enable_valgrind" = xyes])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 ccan/Makefile
                ])
AC_CONFIG_SUBDIRS([ccan
                  ])
AC_OUTPUT
