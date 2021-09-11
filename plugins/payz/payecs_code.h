#ifndef LIGHTNING_PLUGINS_PAYZ_PAYECS_CODE_H
#define LIGHTNING_PLUGINS_PAYZ_PAYECS_CODE_H
#include"config.h"
#include<plugins/libplugin.h>
#include<stddef.h>

/*~ This module implements a set of commands for causing
 * the ECS to invoke code on your plugins, i.e. the
 * "system" part of ECS.
 */

extern const struct plugin_command payecs_code_commands[];
extern const size_t num_payecs_code_commands;

#endif /* LIGHTNING_PLUGINS_PAYZ_PAYECS_CODE_H */
