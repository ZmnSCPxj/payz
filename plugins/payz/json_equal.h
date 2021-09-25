#ifndef LIGHTNING_PLUGINS_PAYZ_JSON_EQUAL_H
#define LIGHTNING_PLUGINS_PAYZ_JSON_EQUAL_H
#include"config.h"
#include<common/json.h>

/** json_equal
 *
 * @brief Compares two JSON datums for equality.
 *
 * @desc Order of object keys is ignored for object-to-object
 * comparisons.
 * Whitespace outside of strings is ignored as well, only the
 * JSON structure is compared.
 */
bool
json_equal(const char *buffer1, const jsmntok_t *tok1,
	   const char *buffer2, const jsmntok_t *tok2);

#endif /* LIGHTNING_PLUGINS_PAYZ_JSON_EQUAL_H */
