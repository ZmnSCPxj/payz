#ifndef LIGHTNING_PLUGINS_PAYZ_TOP_H
#define LIGHTNING_PLUGINS_PAYZ_TOP_H
#include"config.h"
#include<common/json.h>
#include<stdbool.h>

struct ecs;
struct plugin;
struct plugin_command;
struct plugin_hook;
struct plugin_notification;

/** struct payz_top
 *
 * @brief Represents the topmost object of payz, including
 * all the hooks necessary to run as a plugin.
 */
struct payz_top {
	/** disablempp
	 *
	 * @brief A flag which if set will disable multipath.
	 */
	bool disablempp;

	/** ecs
	 *
	 * @brief the entity component system framework.
	 */
	struct ecs *ecs;

	/** default_systems
	 *
	 * @brief the default built-in systems which operate
	 * normal payments.
	 */
	const char **default_systems;

	/** commands
	 *
	 * @brief an array of plugin commands to pass to
	 * `plugin_main`, this and the other arrays below
	 * are either NULL or `tal`-allocated so you ca
	 * use `tal_count`.
	 */
	struct plugin_command *commands;

	/** notifications
	 *
	 * @brief an array of notifications we are interested
	 * in.
	 */
	struct plugin_notification *notifications;

	/** hooks
	 *
	 * @brief an array of hooks we need to install.
	 */
	struct plugin_hook *hooks;

	/** notif_topics
	 *
	 * @brief an array of notification topics we might
	 * publish.
	 */
	const char **notif_topics;
};

/** payz_top
 *
 * @brief The global payz_top object, initialized by
 * setup_payz_top.
 */
extern struct payz_top *payz_top;

/** setup_payz_top
 *
 * @brief Initializes the payz_top global object.
 */
void setup_payz_top(const char *pay_command,
		    const char *keysend_command);
/** shutdown_payz_top
 *
 * @brief Shuts down the payz_top global object.
 */
void shutdown_payz_top(void);

/** payz_top_init
 *
 * @brief To be called during the `init` method.
 */
const char *payz_top_init(struct plugin *plugin,
			  const char *buffer,
			  const jsmntok_t *tok);

#endif /* LIGHTNING_PLUGINS_PAYZ_TOP_H */
