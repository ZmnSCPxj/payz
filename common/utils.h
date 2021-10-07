#ifndef PAYZ_COMMON_UTILS_H
#define PAYZ_COMMON_UTILS_H
#include"config.h"
#include<ccan/mem/mem.h>
#include<ccan/tal/tal.h>

/* This shim exists to remove dependencies on libwally and
 * libsepc256k1.
 */

/* We do not need this here, but common/json.h expects that
 * *something* will include build_assert.h, and may depend
 * on common/utils.h including ccan/ccan/structeq/structeq.h,
 * which includes this.  */
#include<ccan/build_assert/build_assert.h>

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

/* Check for valid UTF-8 */
bool utf8_check(const void *buf, size_t buflen);

#define secp256k1_ctx NULL

#define SECP256K1_TAG_PUBKEY_EVEN 0x02
#define SECP256K1_TAG_PUBKEY_ODD 0x03

/**
 * Remove an element from an array
 *
 * This will shift the elements past the removed element, changing
 * their position in memory, so only use this for arrays of pointers.
 */
#define tal_arr_remove(p, n) tal_arr_remove_((p), sizeof(**p), (n))
void tal_arr_remove_(void *p, size_t elemsize, size_t n);

#endif /* PAYZ_COMMON_UTILS_H */
