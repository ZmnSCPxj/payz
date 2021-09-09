#include"top.h"
#include<assert.h>
#include<plugins/libplugin.h>
#include<plugins/payz/ecs/ecs.h>
#include<plugins/payz/payecs_data.h>

struct payz_top *payz_top = NULL;

void setup_payz_top(const char *pay_command,
		    const char *keysend_command)
{
	assert(!payz_top);
	payz_top = tal(NULL, struct payz_top);

	payz_top->disablempp = false;
	payz_top->ecs = ecs_new(payz_top);

	payz_top->commands = tal_arr(payz_top, struct plugin_command, 0);
	tal_expand(&payz_top->commands,
		   payecs_data_commands, num_payecs_data_commands);

	payz_top->notifications = NULL; /* TODO.  */
	payz_top->hooks = NULL; /* TODO.  */
	payz_top->notif_topics = NULL; /* TODO.  */
}

void shutdown_payz_top(void)
{
	assert(payz_top);
	payz_top = tal_free(payz_top);
}


const char *payz_top_init(struct plugin *plugin,
			  const char *buffer,
			  const jsmntok_t *tok)
{
	/* TODO.  */
	return NULL;
}
