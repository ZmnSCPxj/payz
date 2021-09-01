#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([PAYZCCAN], [0.1A], [ZmnSCPxj@protonmail.com])
AC_CONFIG_AUX_DIR([auxdir])
AM_INIT_AUTOMAKE([subdir-objects tar-ustar foreign])
AC_CONFIG_SRCDIR([ccan/alignof/alignof.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
LT_INIT([disable-shared])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h stddef.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

AH_BOTTOM([
#include"compat_config.h"
])

# __alignof__ support.
AC_MSG_CHECKING([if __alignof__ is supported])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
	int check[__alignof__(double) > 0 ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_ALIGNOF], [1], [Define to 1 if the compiler supports __alignof__])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __typeof__ support.
AC_MSG_CHECKING([if __typeof__ is supported])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
	int example = 0;
	int check[((__typeof__(example) *)0 == (int*)0) ? 1 : -1];
	(void) example;
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_TYPEOF], [1], [Define to 1 if the compiler supports __typeof__])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# isblank in ctype.h
AC_MSG_CHECKING([for isblank in ctype.h])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include<ctype.h>
]], [[
	(void) isblank(' ');
]])], [ # then
    AC_DEFINE([HAVE_ISBLANK], [1], [Define to 1 if isblank() exists in ctype.h with _GNU_SOURCE defined])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__cold__))
AC_MSG_CHECKING([for __attribute__((__cold__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern void __attribute__((__cold__)) check(void);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_COLD], [1], [Define to 1 if compiler supports __attribute__((__cold__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__noreturn__))
AC_MSG_CHECKING([for __attribute__((__noreturn__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern void __attribute__((__noreturn__)) check(void);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_NORETURN], [1], [Define to 1 if compiler supports __attribute__((__noreturn__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((format(__printf__, ...)))
AC_MSG_CHECKING([for __attribute__((format(__printf__, ...)))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern void __attribute__((format(__printf__, 1, 2)))
check(const char *fmt, ...);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_PRINTF], [1], [Define to 1 if compiler supports __attribute__((format(__printf__, nfmt, narg)))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__const__))
AC_MSG_CHECKING([for __attribute__((__const__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int __attribute__((__const__)) check(int);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_CONST], [1], [Define to 1 if compiler supports __attribute__((__const__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__pure__))
AC_MSG_CHECKING([for __attribute__((__pure__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int __attribute__((__pure__)) check(int);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_PURE], [1], [Define to 1 if compiler supports __attribute__((__pure__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__unused__))
AC_MSG_CHECKING([for __attribute__((__unused__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int __attribute__((__unused__)) check(int);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_UNUSED], [1], [Define to 1 if compiler supports __attribute__((__unused__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__used__))
AC_MSG_CHECKING([for __attribute__((__used__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int __attribute__((__used__)) check(int);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_USED], [1], [Define to 1 if compiler supports __attribute__((__used__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__warn_unused_result__))
AC_MSG_CHECKING([for __attribute__((__warn_unused_result__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int __attribute__((__warn_unused_result__)) check(int);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_WARN_UNUSED_RESULT], [1], [Define to 1 if compiler supports __attribute__((__warn_unused_result__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__deprecated__))
AC_MSG_CHECKING([for __attribute__((__deprecated__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int __attribute__((__deprecated__)) check(int);
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_DEPRECATED], [1], [Define to 1 if compiler supports __attribute__((__deprecated__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__nonnull__))
AC_MSG_CHECKING([for __attribute__((__nonnull__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int check(int *) __attribute__((__nonnull__));
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_NONNULL], [1], [Define to 1 if compiler supports __attribute__((__nonnull__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __attribute__((__sentinel__))
AC_MSG_CHECKING([for __attribute__((__sentinel__))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
extern int check(int *, ...) __attribute__((__sentinel__));
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_ATTRIBUTE_SENTINEL], [1], [Define to 1 if compiler supports __attribute__((__sentinel__))])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_constant_p
AC_MSG_CHECKING([for __builtin_constant_p])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[__builtin_constant_p(1) ? 1 : -1];
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CONSTANT_P], [1], [Define to 1 if compiler supports __builtin_constant_p])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_cpu_supports
AC_MSG_CHECKING([for __builtin_cpu_supports])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[__builtin_cpu_supports("mmx") ? 1 : -1];
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CPU_SUPPORTS], [1], [Define to 1 if compiler supports __builtin_cpu_supports])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
