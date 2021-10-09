#ifndef LIGHTNING_PLUGINS_PAYZ_SYSTEMS_DEFAULTER_H
#define LIGHTNING_PLUGINS_PAYZ_SYSTEMS_DEFAULTER_H
#include"config.h"
#include<plugins/payz/ecs/ecs.h>

struct plugin;

extern struct ecs_register_desc system_defaulter[];

void system_defaulter_init(struct plugin *plugin);

#endif /* LIGHTNING_PLUGINS_PAYZ_SYSTEMS_DEFAULTER_H */
