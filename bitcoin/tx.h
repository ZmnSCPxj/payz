#ifndef PAYZ_BITCOIN_TX_H
#define PAYZ_BITCOIN_TX_H
#include"config.h"
#include<bitcoin/privkey.h>
#include<bitcoin/shadouble.h>
#include<bitcoin/signature.h>
#include<ccan/tal/tal.h>

/* Shim to get common/channel_id.c and common/json_helpers.c
 * compiling.
 * channel_id really only needs the TXID below, and we
 * really only need channel_id to get common/json_helpers.c
 * compiling.
 */

struct bitcoin_txid {
	struct sha256_double shad;
};

struct bitcoin_tx { };
struct bitcoin_outpoint {
	struct bitcoin_txid txid;
	u32 n;
};

#define linearize_tx(ctx, tx) tal_arr((ctx), char*, 0)

bool bitcoin_txid_from_hex(const char *hexstr, size_t hexstr_len,
			   struct bitcoin_txid *txid);
bool bitcoin_txid_to_hex(const struct bitcoin_txid *txid,
			 char *hexstr, size_t hexstr_len);

#endif /* PAYZ_BITCOIN_TX_H */
