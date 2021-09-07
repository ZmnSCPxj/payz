#include"gossip_store.h"

/* Shim for common/gossmap.c.
 * While we want to use the gossmap, gossmap itself
 * only uses the structs and constants in
 * common/gossip_store.h, not any of the functions.
 */
