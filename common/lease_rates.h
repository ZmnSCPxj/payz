#ifndef PAYZ_COMMON_LEASE_RATES_H
#define PAYZ_COMMON_LEASE_RATES_H
#include"config.h"
#include<wire/wire.h>
#include<stddef.h>

/* Shim to get common/json_tok.c compiling.
 * Payment algorithm does not actually use lease rates.
 */

#define lease_rates_fromhex(ctx, buffer, tok) NULL

#endif /* PAYZ_COMMON_LEASE_RATES_H */
