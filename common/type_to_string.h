#ifndef PAYZ_COMMON_TYPE_TO_STRING_H
#define PAYZ_COMMON_TYPE_TO_STRING_H
#include"config.h"
#include<ccan/autodata/autodata.h>
#include<ccan/check_type/check_type.h>
#include<ccan/str/str.h>
#include<ccan/tal/tal.h>
#include<common/utils.h>

/* This shim exists so that we do not have to bring in all the dependencies
 * (particularly libwally and libsecp256k1) that the lightningd
 * type_to_string module has.
 *
 * The main reason why the lightningd version has the union printable_types
 * is because some types are `struct foo`, but other types, particularly the
 * secp256k1 types, are typedefs and are just `foo`.
 * Since payz does not use secp256k1 itself, it can afford to just use
 * void* for simplicity, and assumes that all type names are `struct foo`.
 */

struct type_to_string {
	const char *typename;
	const char *(*fmt)(const tal_t *ctx, const void *ptr);
};

#define REGISTER_TYPE_TO_STRING(typename, fmtfn) \
	static const char *fmt_##typename##_(const tal_t *ctx, const void *ptr) \
	{ \
		return fmtfn(ctx, (const struct typename *) ptr); \
	} \
	static struct type_to_string ttos_##typename = { \
		#typename, fmt_##typename##_ \
	} \
	AUTODATA(type_to_string, &ttos_##typename)

#define REGISTER_TYPE_TO_HEXSTR(typename) \
	static const char *fmt_##typename##_(const tal_t *ctx, const void *ptr) \
	{ \
		return tal_hexstr(ctx, ptr, sizeof(struct typename)); \
	} \
	static struct type_to_string ttos_##typename = { \
		#typename, fmt_##typename##_ \
	} \
	AUTODATA(type_to_string, &ttos_##typename)

#define type_to_string(ctx, type, ptr) \
	type_to_string_((ctx), stringify(type), ptr + check_type(ptr, type))

const char *type_to_string_(const tal_t *ctx, const char *typename,
			    const void *ptr);

AUTODATA_TYPE(type_to_string, struct type_to_string);

#endif /* PAYZ_COMMON_TYPE_TO_STRING_H */
