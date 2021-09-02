#ifndef PAYZ_WIRE_WIRE_H
#define PAYZ_WIRE_WIRE_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<inttypes.h>

/* Shim to remove dependencies on libsecp256k1 and libwally.
 *
 * While ideally payz, as a plugin, communicates via JSON-RPC
 * and not a binary wire protocol, binary blobs do get sent
 * back and forth, which need to get extracted/composed by the
 * wire mechanism.
 */

void towire(u8 **pptr, const void *data, size_t len);
const u8 *fromwire(const u8 **cursor, size_t *max, void *copy, size_t n);
void *fromwire_fail(const u8 **cursor, size_t *max);
int fromwire_peektype(const u8 *cursor);

u64 fromwire_u64(const u8 **cursor, size_t *max);
void towire_u64(u8 **pptr, u64 v);

#endif /* PAYZ_WIRE_WIRE_H */
