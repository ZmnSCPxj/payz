#include"wire.h"
#include<assert.h>
#include<ccan/build_assert/build_assert.h>
#include<ccan/endian/endian.h>
#include<ccan/mem/mem.h>
#include<string.h>

#ifndef SUPERVERBOSE
#define SUPERVERBOSE(x)
#endif

/* Sets *cursor to NULL and returns NULL when extraction fails. */
void *fromwire_fail(const u8 **cursor, size_t *max)
{
	*cursor = NULL;
	*max = 0;
	return NULL;
}

const u8 *fromwire(const u8 **cursor, size_t *max, void *copy, size_t n)
{
	const u8 *p = *cursor;

	if (*max < n) {
		/* Just make sure we don't leak uninitialized mem! */
		if (copy)
			memset(copy, 0, n);
		if (*cursor)
			SUPERVERBOSE("less than encoding length");
		return fromwire_fail(cursor, max);
	}
	*cursor += n;
	*max -= n;
	if (copy)
		memcpy(copy, p, n);
	return memcheck(p, n);
}
void towire(u8 **pptr, const void *data, size_t len)
{
	size_t oldsize = tal_count(*pptr);

	tal_resize(pptr, oldsize + len);
	memcpy(*pptr + oldsize, memcheck(data, len), len);
}

static u64 fromwire_tlv_uint(const u8 **cursor, size_t *max, size_t maxlen)
{
	u8 bytes[8];
	size_t length;
	be64 val;

	assert(maxlen <= sizeof(bytes));

	/* BOLT #1:
	 *
	 * - if `length` is not exactly equal to that required for the
	 *   known encoding for `type`:
	 *	- MUST fail to parse the `tlv_stream`.
	 */
	length = *max;
	if (length > maxlen) {
		SUPERVERBOSE("greater than encoding length");
		fromwire_fail(cursor, max);
		return 0;
	}

	memset(bytes, 0, sizeof(bytes));
	fromwire(cursor, max, bytes + sizeof(bytes) - length, length);

	/* BOLT #1:
	 * - if variable-length fields within the known encoding for `type` are
	 *   not minimal:
	 *    - MUST fail to parse the `tlv_stream`.
	 */
	if (length > 0 && bytes[sizeof(bytes) - length] == 0) {
		SUPERVERBOSE("not minimal");
		fromwire_fail(cursor, max);
		return 0;
	}
	BUILD_ASSERT(sizeof(val) == sizeof(bytes));
	memcpy(&val, bytes, sizeof(bytes));
	return be64_to_cpu(val);
}
static void towire_tlv_uint(u8 **pptr, u64 v)
{
	u8 bytes[8];
	size_t num_zeroes;
	be64 val;

	val = cpu_to_be64(v);
	BUILD_ASSERT(sizeof(val) == sizeof(bytes));
	memcpy(bytes, &val, sizeof(bytes));

	for (num_zeroes = 0; num_zeroes < sizeof(bytes); num_zeroes++)
		if (bytes[num_zeroes] != 0)
			break;

	towire(pptr, bytes + num_zeroes, sizeof(bytes) - num_zeroes);
}

int fromwire_peektype(const u8 *cursor)
{
	be16 be_type;
	size_t max = tal_count(cursor);

	fromwire(&cursor, &max, &be_type, sizeof(be_type));
	if (!cursor)
		return -1;
	return be16_to_cpu(be_type);
}

u64 fromwire_tu64(const u8 **cursor, size_t *max)
{
	return fromwire_tlv_uint(cursor, max, 8);
}
u64 fromwire_u64(const u8 **cursor, size_t *max)
{
	be64 ret;

	if (!fromwire(cursor, max, &ret, sizeof(ret)))
		return 0;
	return be64_to_cpu(ret);
}
u16 fromwire_u16(const u8 **cursor, size_t *max)
{
	be16 ret;

	if (!fromwire(cursor, max, &ret, sizeof(ret)))
		return 0;
	return be16_to_cpu(ret);
}
u8 fromwire_u8(const u8 **cursor, size_t *max)
{
	u8 ret;

	if (!fromwire(cursor, max, &ret, sizeof(ret)))
		return 0;
	return ret;
}
bool fromwire_bool(const u8 **cursor, size_t *max)
{
	u8 ret;

	if (!fromwire(cursor, max, &ret, sizeof(ret)))
		return false;
	if (ret != 0 && ret != 1)
		fromwire_fail(cursor, max);
	return ret;
}
void fromwire_sha256(const u8 **cursor, size_t *max, struct sha256 *sha256)
{
        fromwire(cursor, max, sha256, sizeof(*sha256));
}
void fromwire_u8_array(const u8 **cursor, size_t *max, u8 *arr, size_t num)
{
	fromwire(cursor, max, arr, num);
}
u8 *fromwire_tal_arrn(const tal_t *ctx,
		      const u8 **cursor, size_t *max, size_t num)
{
	u8 *arr;
	if (num == 0)
		return NULL;

	if (num > *max)
		return fromwire_fail(cursor, max);

	arr = tal_arr(ctx, u8, num);
	fromwire_u8_array(cursor, max, arr, num);
	return arr;
}

void towire_tu64(u8 **pptr, u64 v)
{
	return towire_tlv_uint(pptr, v);
}
void towire_u64(u8 **pptr, u64 v)
{
	be64 l = cpu_to_be64(v);
	towire(pptr, &l, sizeof(l));
}
void towire_u16(u8 **pptr, u16 v)
{
	be16 l = cpu_to_be16(v);
	towire(pptr, &l, sizeof(l));
}
void towire_u8(u8 **pptr, u8 v)
{
	towire(pptr, &v, sizeof(v));
}
void towire_bool(u8 **pptr, bool v)
{
	u8 val = v;
	towire(pptr, &val, sizeof(val));
}
void towire_sha256(u8 **pptr, const struct sha256 *sha256)
{
        towire(pptr, sha256, sizeof(*sha256));
}
void towire_u8_array(u8 **pptr, const u8 *arr, size_t num)
{
	towire(pptr, arr, num);
}
