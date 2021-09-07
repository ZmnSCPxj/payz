#ifndef PAYZ_BITCOIN_SIGNATURE_H
#define PAYZ_BITCOIN_SIGNATURE_H
#include"config.h"
#include<ccan/short_types/short_types.h>

/* Shim to get common/json_helpers.c compiling.
 * We do not actually need bip340sig here, as the
 * pay algorithm does not touch onchain.
 */
struct bip340sig {
	u8 u8[64];
};

#endif /* PAYZ_BITCOIN_SIGNATURE_H */
