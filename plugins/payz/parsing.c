#include"parsing.h"
#include"ccan/json_escape/json_escape.h"
#include"common/json_command.h"

struct command_result *
param_array_of_strings(struct command *cmd, const char *name,
		       const char *buffer, const jsmntok_t *tok,
		       const char ***arr)
{
	struct json_escape *esc;
	const jsmntok_t *j_str;
	size_t i;

	/* As a convenience, if a single string, promote to a 1-entry
	 * array of strings.  */
	if (tok->type == JSMN_STRING) {
		*arr = tal_arr(cmd, const char *, 1);
		esc = json_escape_string_(cmd, buffer + tok->start,
					  tok->end - tok->start);
		(*arr)[0] = json_escape_unescape(cmd, esc);
		if (!(*arr)[0])
			goto fail;
		return NULL;
	}

	if (tok->type != JSMN_ARRAY)
		goto fail;

	*arr = tal_arr(cmd, const char *, tok->size);
	json_for_each_arr (i, j_str, tok) {
		esc = json_escape_string_(cmd, buffer + j_str->start,
					  j_str->end - j_str->start);
		(*arr)[i] = json_escape_unescape(cmd, esc);
		if (!(*arr)[i])
			goto fail;
	}
	return NULL;

fail:
	return command_fail_badparam(cmd, name, buffer, tok,
				     "should be an array of strings.");
}
