#ifndef CCAN_COMPAT_CONFIG_H
#define CCAN_COMPAT_CONFIG_H

/* Always use GNU extensions.  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/* The test in ccan/tools/configurator.c uses __typeof__,
 * but the code in ccan/check_type/check_type.h uses
 * typeof, avoid any issues by defining typeof as the
 * tested __typeof__.
 */
#if HAVE_TYPEOF
#define typeof __typeof__
#endif

/* AC_C_BIGENDIAN defines WORDS_BIGENDIAN, convert to the
 * ccan-expected HAVE_LITTLE_ENDIAN and HAVE_BIG_ENDIAN.
 * In particular this handles the Apple "Universal" binary
 * build case.
 */
#if WORDS_BIGENDIAN
#define HAVE_BIG_ENDIAN 1
#else
#define HAVE_LITTLE_ENDIAN 1
#endif

/* AX_CHECK_ALIGNED_ACCESS_REQUIRED defines
 * HAVE_ALIGNED_ACCESS_REQUIRED, and also has the
 * opposite polarity.
 * Arguably the AX_* version has a risky polarity, since
 * autoheader generates HAVE_ALIGNED_ACCESS_REQUIRED
 * unset, which is the *unsafe* setting!
 * Sigh.
 */
#if !HAVE_ALIGNED_ACCESS_REQUIRED
#define HAVE_UNALIGNED_ACCESS 1
#endif

/* Use JSMN in "separate mode", i.e. define everything
 * extern.
 */
#define JSMN_HEADER
#undef JSMN_STATIC

#endif /* !defined(CCAN_COMPAT_CONFIG_H) */
