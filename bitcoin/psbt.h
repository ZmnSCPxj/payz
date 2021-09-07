#ifndef PAYZ_BITCOIN_PSBT_H
#define PAYZ_BITCOIN_PSBT_H
#include"config.h"
#include<ccan/tal/tal.h>
#include<stddef.h>

/* Shim to get common/json_helpers.c compiling.
 * Lightning-level paymants do not need to touch
 * onchain-level transactions.
 */

struct wally_psbt { };

struct wally_psbt *psbt_from_b64(const tal_t *ctx,
				 const char *b64,
				 size_t b64len);
char *psbt_to_b64(const tal_t *ctx, const struct wally_psbt *psbt);

#endif /* PAYZ_BITCOIN_PSBT_H */
