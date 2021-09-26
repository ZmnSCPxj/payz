#include"parsing.h"
#include"ccan/json_escape/json_escape.h"
#include"common/json_command.h"
#include"common/utils.h"

const char **
json_to_array_of_strings(const tal_t *ctx,
			 const char *buffer, const jsmntok_t *tok)
{
	const char **arr;

	struct json_escape *esc;
	const jsmntok_t *j_str;
	size_t i;

	/* As a convenience, if a single string, promote to a 1-entry
	 * array of strings.  */
	if (tok->type == JSMN_STRING) {
		arr = tal_arr(ctx, const char *, 1);
		esc = json_escape_string_(tmpctx, buffer + tok->start,
					  tok->end - tok->start);
		arr[0] = json_escape_unescape(arr, esc);
		if (!arr[0])
			return tal_free(arr);
		return arr;
	}

	if (tok->type != JSMN_ARRAY)
		return NULL;

	arr = tal_arr(ctx, const char *, tok->size);
	json_for_each_arr(i, j_str, tok) {
		if (j_str->type != JSMN_STRING)
			return tal_free(arr);
		esc = json_escape_string_(tmpctx, buffer + j_str->start,
					   j_str->end - j_str->start);
		arr[i] = json_escape_unescape(arr, esc);
		if (!arr[i])
			return tal_free(arr);
	}

	return arr;
}

struct command_result *
param_array_of_strings(struct command *cmd, const char *name,
		       const char *buffer, const jsmntok_t *tok,
		       const char ***arr)
{
	*arr = json_to_array_of_strings(cmd, buffer, tok);
	if (*arr)
		return NULL;
	return command_fail_badparam(cmd, name, buffer, tok,
				     "should be an array of strings.");
}
