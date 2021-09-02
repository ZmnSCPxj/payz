#ifndef PAYZ_COMMON_UTILS_H
#define PAYZ_COMMON_UTILS_H
#include"config.h"
#include<ccan/tal/tal.h>

/* This shim exists to remove dependencies on libwally and
 * libsepc256k1.
 */

struct chainparams;

extern const struct chainparams *chainparams;

#define STEALS

char *tal_hexstr(const tal_t *ctx, const void *data, size_t len);

void setup_locale(void);

extern const tal_t *tmpctx;
void setup_tmpctx(void);
void clean_tmpctx(void);

#endif /* PAYZ_COMMON_UTILS_H */
