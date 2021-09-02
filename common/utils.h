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

/* Note: p is never a complex expression, otherwise this multi-evaluates! */
#define tal_arr_expand(p, s)						\
	do {								\
		size_t n_ = tal_count(*(p));				\
		tal_resize((p), n_+1);					\
		(*(p))[n_] = (s);					\
	} while(0)

/**
 * The comon case of duplicating an entire tal array.
 *
 * A macro because we must not double-evaluate p.
 */
#define tal_dup_talarr(ctx, type, p)					\
	((type *)tal_dup_talarr_((ctx), tal_typechk_(p, type *),	\
				 TAL_LABEL(type, "[]")))
void *tal_dup_talarr_(const tal_t *ctx, const tal_t *src TAKES,
		      const char *label);

#endif /* PAYZ_COMMON_UTILS_H */
