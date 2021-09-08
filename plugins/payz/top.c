#include"top.h"
#include<assert.h>
#include<plugins/libplugin.h>
#include<plugins/payz/ecs/ecs.h>

struct payz_top *payz_top = NULL;

void setup_payz_top(const char *pay_command,
		    const char *keysend_command)
{
	assert(!payz_top);
	payz_top = tal(NULL, struct payz_top);

	payz_top->disablempp = false;
	payz_top->ecs = ecs_new(payz_top);
	payz_top->commands = NULL; /* TODO.  */
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
