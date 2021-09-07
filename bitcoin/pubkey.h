#ifndef PAYZ_BITCOIN_PUBKEY_H
#define PAYZ_BITCOIN_PUBKEY_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<stdbool.h>
#include<string.h>

/* Shim to get common/json_helpers.c and common/node_id.c
 * compiling.
 * While payz does need to store node IDs, it does not
 * really need Bitcoin pubkey.
 */

#define PUBKEY_CMPR_LEN 33

struct pubkey { };
struct pubkey32 { };

#define pubkey_from_hexstr(der, derlen, out) false
#define pubkey_to_der(a, in) memset((void *)(a), 0, PUBKEY_CMPR_LEN)
#define secp256k1_xonly_pubkey_serialize(ctx, a, in)  memset((void *)(a), 0, 32)
#define secp256k1_ec_pubkey_serialize(a, b, c, d, e) ((c) == NULL)
#define secp256k1_ec_pubkey_parse(a, b, c, d) false
#define secp256k1_xonly_pubkey_from_pubkey(a, b, c, d) false
#define pubkey_idx(a, b) 0

#endif /* PAYZ_BITCOIN_PUBKEY_H */
