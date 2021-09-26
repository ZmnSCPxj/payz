#ifndef LIGHTNING_PLUGINS_PAYZ_PARSING_H
#define LIGHTNING_PLUGINS_PAYZ_PARSING_H
#include"config.h"
#include<common/json.h>
#include<ccan/tal/tal.h>

struct command;
struct command_result;

struct command_result *
param_array_of_strings(struct command *cmd, const char *name,
		       const char *buffer, const jsmntok_t *tok,
		       const char ***arr);

const char **
json_to_array_of_strings(const tal_t *ctx,
			 const char *buffer, const jsmntok_t *tok);

#endif /* LIGHTNING_PLUGINS_PAYZ_PARSING_H */
