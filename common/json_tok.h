#ifndef PAYZ_COMMON_JSON_TOK_H
#define PAYZ_COMMON_JSON_TOK_H
#include"config.h"
#include<common/json.h>

/* plugins/libplugin.h does not include these, expecting
 * these to be included *somewhere* in the inclusion
 * graph.
 */
#include<common/amount.h>
#include<common/node_id.h>

/* Shim for getting a number of things to compile.
 * We really only need a small number of things to
 * be parsable here, so we add them as needed only.
 */

struct command;

struct command_result *param_string(struct command *cmd, const char *name,
				    const char * buffer, const jsmntok_t *tok,
				    const char **str);

#endif /* PAYZ_COMMON_JSON_TOK_H */
