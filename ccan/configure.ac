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

AC_C_BIGENDIAN

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

# __attribute__((__section__))
AC_MSG_CHECKING([for __attribute__((__section__)) and __start/__stop])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	static void *__attribute__((__section__("mysec"))) p = &p;
	static int func() {
		extern void *__start_mysec[], *__stop_mysec[];
		return __stop_mysec - __start_mysec;
	}
]], [[(void)&func;]])], [ # then
    AC_DEFINE([HAVE_SECTION_START_STOP], [1], [Define to 1 if compiler supports __attribute__((__section__)) and __start/__stop])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_constant_p
AC_MSG_CHECKING([for __builtin_constant_p])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[__builtin_constant_p(1) ? 1 : -1];
	(void) check;
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
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CPU_SUPPORTS], [1], [Define to 1 if compiler supports __builtin_cpu_supports])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_expect
AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[__builtin_expect(1 == 1, 1) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_EXPECT], [1], [Define to 1 if compiler supports __builtin_expect])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_choose_expr
AC_MSG_CHECKING([for __builtin_choose_expr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_choose_expr(1, 1, 0) && __builtin_choose_expr(0, 0, 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CHOOSE_EXPR], [1], [Define to 1 if compiler supports __builtin_choose_expr])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_types_compatible_p
AC_MSG_CHECKING([for __builtin_types_compatible_p])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[!__builtin_types_compatible_p(char*, int) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_TYPES_COMPATIBLE_P], [1], [Define to 1 if compiler supports __builtin_types_compatible_p])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_clz
AC_MSG_CHECKING([for __builtin_clz])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_clz(1) == (sizeof(int) * 8 - 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CLZ], [1], [Define to 1 if compiler supports __builtin_clz])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# __builtin_clzl
AC_MSG_CHECKING([for __builtin_clzl])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_clzl(1) == (sizeof(long) * 8 - 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CLZL], [1], [Define to 1 if compiler supports __builtin_clzl])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# __builtin_clzll
AC_MSG_CHECKING([for __builtin_clzll])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_clzll(1) == (sizeof(long long) * 8 - 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CLZLL], [1], [Define to 1 if compiler supports __builtin_clzll])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_ctz
AC_MSG_CHECKING([for __builtin_ctz])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_ctz(1 << (sizeof(int) * 8 - 1)) == (sizeof(int) * 8 - 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CTZ], [1], [Define to 1 if compiler supports __builtin_ctz])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# __builtin_ctzl
AC_MSG_CHECKING([for __builtin_ctzl])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_ctzl(1 << (sizeof(long) * 8 - 1)) == (sizeof(long) * 8 - 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CTZL], [1], [Define to 1 if compiler supports __builtin_ctzl])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# __builtin_ctzll
AC_MSG_CHECKING([for __builtin_ctzll])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_ctzll(1 << (sizeof(long long) * 8 - 1)) == (sizeof(long long) * 8 - 1)) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_CTZLL], [1], [Define to 1 if compiler supports __builtin_ctzll])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# __builtin_ffs
AC_MSG_CHECKING([for __builtin_ffs])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_ffs(0) == 0) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_FFS], [1], [Define to 1 if compiler supports __builtin_ffs])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# __builtin_ffsl
AC_MSG_CHECKING([for __builtin_ffsl])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_ffsl(0L) == 0) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_FFSL], [1], [Define to 1 if compiler supports __builtin_ffsl])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# __builtin_ffsll
AC_MSG_CHECKING([for __builtin_ffsll])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[
	int check[(__builtin_ffsll(0LL) == 0) ? 1 : -1];
	(void) check;
]])], [ # then
    AC_DEFINE([HAVE_BUILTIN_FFSLL], [1], [Define to 1 if compiler supports __builtin_ffsll])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# <byteswap.h>
AC_MSG_CHECKING([for byteswap.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#include<byteswap.h>
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_BYTESWAP_H], [1], [Define to 1 if <byteswap.h> exists and contains byteswap functions.])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])
# bswap_64 in <byteswap.h>
AC_MSG_CHECKING([for bswap_64 in byteswap.h])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	#include<byteswap.h>
	#include<stdint.h>
]], [[
	int64_t x = 42;
	int64_t y = bswap_64(x);
	(void) y;
]])], [ # then
    AC_DEFINE([HAVE_BSWAP_64], [1], [Define to 1 if <byteswap.h> exists and contains bswap_64.])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# If flexible array size is allowed for last field of
# structure.
AC_MSG_CHECKING([if flexible array size is allowed for last struct member])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	struct check { unsigned int x; int arr[]; };
]], [[ ]])], [ # then
    AC_DEFINE([HAVE_FLEXIBLE_ARRAY_MEMBER], [1], [Define to 1 if compiler allows empty array size for last struct member.])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# struct timespec
AC_MSG_CHECKING([for struct timespec in time.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#include<time.h>
]], [[
	struct timespec ts;
	ts.tv_sec = ts.tv_nsec = 1;
	(void) ts;
]])], [ # then
    AC_DEFINE([HAVE_STRUCT_TIMESPEC], [1], [Define to 1 if <time.h> exists and defines struct timespec.])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# clock_gettime
AC_MSG_CHECKING([for clock_gettime in time.h])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	#include<time.h>
]], [[
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	(void) ts;
]])], [ # then
    AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [Define to 1 if <time.h> exists and declares clock_gettime.])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# <err.h>
AC_MSG_CHECKING([for err.h])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	#include<err.h>
	static void check(int arg) {
		if (arg == 0)
			err(1, "err %d", arg);
		if (arg == 1)
			errx(1, "err %d", arg);
		if (arg == 3)
			warn("warn %d", arg);
		if (arg == 4)
			warnx("warn %d", arg);
	}
]], [[(void)&check;]])], [ # then
    AC_DEFINE([HAVE_ERR_H], [1], [Define to 1 if <err.h> exists and contains BSD err functions.])
    AC_MSG_RESULT([yes])
], [ # else
    AC_MSG_RESULT([no])
])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
