#ifndef PAYZ_BITCOIN_TX_H
#define PAYZ_BITCOIN_TX_H
#include"config.h"
#include<bitcoin/shadouble.h>

/* Shim to get common/channel_id.c compiling.
 * channel_id really only needs the TXID below, and we
 * really only need channel_id to get common/json_helpers.c
 * compiling.
 */

struct bitcoin_txid {
	struct sha256_double shad;
};

#endif /* PAYZ_BITCOIN_TX_H */
