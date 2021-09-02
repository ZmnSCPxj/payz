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
u16 fromwire_u16(const u8 **cursor, size_t *max);
void towire_u64(u8 **pptr, u64 v);
void towire_u16(u8 **pptr, u16 v);

u8 *fromwire_tal_arrn(const tal_t *ctx,
		       const u8 **cursor, size_t *max, size_t num);
void fromwire_u8_array(const u8 **cursor, size_t *max, u8 *arr, size_t num);
void towire_u8_array(u8 **pptr, const u8 *arr, size_t num);

#endif /* PAYZ_WIRE_WIRE_H */
