#include"top.h"
#include<assert.h>
#include<ccan/tal/str/str.h>
#include<common/utils.h>
#include<plugins/libplugin.h>
#include<plugins/payz/ecs/ecs.h>
#include<plugins/payz/payecs_code.h>
#include<plugins/payz/payecs_data.h>
#include<plugins/payz/systems/nonce.h>

struct payz_top *payz_top = NULL;

void setup_payz_top(const char *pay_command,
		    const char *keysend_command)
{
	struct ecs_register_desc *to_register;
	struct ecs_register_desc *p;

	assert(!payz_top);
	payz_top = tal(NULL, struct payz_top);

	payz_top->disablempp = false;
	payz_top->ecs = ecs_new(payz_top);

	payz_top->commands = tal_arr(payz_top, struct plugin_command, 0);
	tal_expand(&payz_top->commands,
		   payecs_data_commands, num_payecs_data_commands);
	tal_expand(&payz_top->commands,
		   payecs_code_commands, num_payecs_code_commands);

	/* Register builtin systems.  */
	to_register = ecs_register_begin(payz_top);
	/* Systems that are included in default.  */
	ecs_register_concat(&to_register, system_nonce);

	/* Extract default systems.  */
	payz_top->default_systems = tal_arr(payz_top, const char *, 0);
	for (p = to_register; p->type != ECS_REGISTER_TYPE_OVER_AND_OUT; ++p) {
		if (p->type != ECS_REGISTER_TYPE_NAME)
			continue;
		tal_arr_expand(&payz_top->default_systems,
			       tal_strdup(payz_top, (const char *) p->pointer));
	}

	/* Systems that are not included in default.  */
	/* TODO: ecs_register_concat(&to_register, some_builtin_system); */

	ecs_register(payz_top->ecs, take(to_register));

	payz_top->notifications = tal_arr(payz_top,
					  struct plugin_notification,
					  0);
	tal_expand(&payz_top->notifications,
		   payecs_code_notifications, num_payecs_code_notifications);

	payz_top->hooks = NULL; /* TODO.  */

	payz_top->notif_topics = tal_arr(payz_top, const char *, 0);
	tal_expand(&payz_top->notif_topics,
		   payecs_code_topics, num_payecs_code_topics);
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
