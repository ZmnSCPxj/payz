/* Shim to get common/json_helpers.c compiling.
 * Lightning-level paymants do not need to touch
 * onchain-level transactions.
 */
#include"bitcoin/psbt.h"
