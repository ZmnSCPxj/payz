#include"type_to_string.h"
#include<assert.h>
#include<ccan/tal/str/str.h>
#include<inttypes.h>

const char *type_to_string_(const tal_t *ctx, const char *typename,
			    const void *ptr)
{
	const char *s = NULL;
	size_t i;
	static size_t num_p;
	static struct type_to_string **t = NULL;

	assert(typename != NULL);

	if (!t)
		t = autodata_get(type_to_string, &num_p);

	if (strstarts(typename, "struct "))
		typename += strlen("struct ");

	for (i = 0; i < num_p; ++i) {
		if (streq(t[i]->typename, typename)) {
			s = t[i]->fmt(ctx, ptr);
			break;
		}
	}

	if (!s)
		s = tal_fmt(ctx, "UNKNOWN TYPE %s %p", typename, ptr);

	return s;
}
