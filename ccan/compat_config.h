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

#endif /* !defined(CCAN_COMPAT_CONFIG_H) */
