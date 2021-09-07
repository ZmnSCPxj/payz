#ifndef PAYZ_WIRE_WIRE_H
#define PAYZ_WIRE_WIRE_H
#include"config.h"
#include<ccan/crypto/sha256/sha256.h>
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

u64 fromwire_tu64(const u8 **cursor, size_t *max);
u64 fromwire_u64(const u8 **cursor, size_t *max);
u16 fromwire_u16(const u8 **cursor, size_t *max);
u8 fromwire_u8(const u8 **cursor, size_t *max);
bool fromwire_bool(const u8 **cursor, size_t *max);
void fromwire_sha256(const u8 **cursor, size_t *max, struct sha256 *sha256);
void towire_tu64(u8 **pptr, u64 v);
void towire_u64(u8 **pptr, u64 v);
void towire_u16(u8 **pptr, u16 v);
void towire_u8(u8 **pptr, u8 v);
void towire_bool(u8 **pptr, bool v);
void towire_sha256(u8 **pptr, const struct sha256 *sha256);

u8 *fromwire_tal_arrn(const tal_t *ctx,
		       const u8 **cursor, size_t *max, size_t num);
void fromwire_u8_array(const u8 **cursor, size_t *max, u8 *arr, size_t num);
void towire_u8_array(u8 **pptr, const u8 *arr, size_t num);

/* This is from generated code, but the payz does not actually
 * use lease rates.
 */
struct lease_rates {
	u16 funding_weight;
	u16 lease_fee_basis;
	u16 channel_fee_max_proportional_thousandths;
	u32 lease_fee_base_sat;
	u32 channel_fee_max_base_msat;
};

#endif /* PAYZ_WIRE_WIRE_H */
