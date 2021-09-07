#ifndef PAYZ_BITCOIN_PUBKEY_H
#define PAYZ_BITCOIN_PUBKEY_H
#include"config.h"
#include<stdbool.h>
#include<string.h>

/* Shim to get common/json_helpers.c compiling.
 * While payz does need to store node IDs, it does not
 * really need Bitcoin pubkey.
 */

#define PUBKEY_CMPR_LEN 33

struct pubkey { };

#define pubkey_from_hexstr(der, derlen, out) false
#define pubkey_to_der(a, in) memset((void *)(a), 0, PUBKEY_CMPR_LEN)
#define secp256k1_xonly_pubkey_serialize(ctx, a, in)  memset((void *)(a), 0, 32)

#endif /* PAYZ_BITCOIN_PUBKEY_H */
