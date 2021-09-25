#include"json_equal.h"
#include<assert.h>

bool
json_equal(const char *buffer1, const jsmntok_t *tok1,
	   const char *buffer2, const jsmntok_t *tok2)
{
	size_t i;
	size_t size;

	const jsmntok_t *key1;
	const jsmntok_t *value1;
	const jsmntok_t *value2;

	assert(buffer1 && tok1 && buffer2 && tok2);

	if (tok1->type != tok2->type)
		return false;

	switch (tok1->type) {
	case JSMN_PRIMITIVE:
	case JSMN_STRING:
		if ((tok1->end - tok1->start) != (tok2->end - tok2->start))
			return false;
		return strncmp(buffer1 + tok1->start,
			       buffer2 + tok2->start,
			       tok1->end - tok1->start) == 0;

	case JSMN_ARRAY:
		if (tok1->size != tok2->size)
			return false;

		/* Now compare both arrays.  */
		size = tok1->size;
		++tok1, ++tok2;
		for (i = 0;
		     i < size;
		     ++i, tok1 = json_next(tok1), tok2 = json_next(tok2)) {
			if (!json_equal(buffer1, tok1, buffer2, tok2))
				return false;
		}

		return true;

	case JSMN_OBJECT:
		if (tok1->size != tok2->size)
			return false;

		/* Now compare both objects.  */
		json_for_each_obj (i, key1, tok1) {
			value1 = key1 + 1;
			value2 = json_get_membern(buffer2, tok2,
						  buffer1 + key1->start,
						  key1->end - key1->start);
			if (!value2)
				return false;
			if (!json_equal(buffer1, value1, buffer2, value2))
				return false;
		}
		return true;

		/* Should never happen.  */
	case JSMN_UNDEFINED:
		abort();
	}
	abort();
}
