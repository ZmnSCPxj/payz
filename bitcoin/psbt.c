#include"psbt.h"
#include<ccan/compiler/compiler.h>
#include<ccan/tal/str/str.h>

/* Shim to get common/json_helpers.c compiling.
 * Lightning-level paymants do not need to touch
 * onchain-level transactions.
 */

struct wally_psbt *psbt_from_b64(const tal_t *ctx,
				 const char *b64 UNUSED,
				 size_t b64len UNUSED)
{
	return tal(ctx, struct wally_psbt);
}
char *psbt_to_b64(const tal_t *ctx, const struct wally_psbt *psbt)
{
	return tal_strdup(ctx, "");
}
