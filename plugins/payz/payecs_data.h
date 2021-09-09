#ifndef LIGHTNING_PLUGINS_PAYZ_PAYECS_DATA_H
#define LIGHTNING_PLUGINS_PAYZ_PAYECS_DATA_H
#include"config.h"
#include<common/errcode.h>
#include<plugins/libplugin.h>
#include<stddef.h>

/*~ This module implements a set of commands for manipulating
 * the ECS.
 */

extern const struct plugin_command payecs_data_commands[];
extern const size_t num_payecs_data_commands;

static const errcode_t PAYECS_SETCOMPONENTS_UNEXPECTED_COMPONENTS = 2244;

#endif /* LIGHTNING_PLUGINS_PAYZ_PAYECS_DATA_H */
