#include"wire.h"
#include<string.h>
#include<ccan/endian/endian.h>
#include<ccan/mem/mem.h>

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

int fromwire_peektype(const u8 *cursor)
{
	be16 be_type;
	size_t max = tal_count(cursor);

	fromwire(&cursor, &max, &be_type, sizeof(be_type));
	if (!cursor)
		return -1;
	return be16_to_cpu(be_type);
}

u64 fromwire_u64(const u8 **cursor, size_t *max)
{
	be64 ret;

	if (!fromwire(cursor, max, &ret, sizeof(ret)))
		return 0;
	return be64_to_cpu(ret);
}
void towire_u64(u8 **pptr, u64 v)
{
	be64 l = cpu_to_be64(v);
	towire(pptr, &l, sizeof(l));
}
