#ifndef PAYZ_COMMON_MEMLEAK_H
#define PAYZ_COMMON_MEMLEAK_H

/* This file is included by common/features.c, but
 * that module does not seem to actually refer to
 * anything in this file.
 * This file is included by common/gossip_rcvd_filter.c,
 * though, and that file refers to some functions (which
 * are disabled in DEVELOPER=0 mode), so add those disabled
 * here.
 */

#define memleak_add_helper(p, cb)

#endif /* PAYZ_COMMON_MEMLEAK_H */
