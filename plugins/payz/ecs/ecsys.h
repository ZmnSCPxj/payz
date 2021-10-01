#ifndef LIGHTNING_PLUGINS_PAYZ_ECS_ECSYS_H
#define LIGHTNING_PLUGINS_PAYZ_ECS_ECSYS_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<ccan/typesafe_cb/typesafe_cb.h>
#include<common/errcode.h>
#include<common/json.h>
#include<stddef.h>

struct command_result;
enum log_level;
struct plugin;

/** struct ecsys
 *
 * @brief Represents a handler for systems in an ECS framework.
 */
struct ecsys;

/** ecsys_new
 *
 * @brief Constructs a new systems handler.
 *
 * @desc This function accepts some API functions as arguments.
 * This allows this handler to be tested independently of its
 * dependencies (the plugin notification code, the EC table
 * object, etc).
 * This is an application of the dependency inversion principle.
 *
 * @param ctx - the owner of this system handler.
 * @param get_component - the function to call to get a component
 * on the EC table.
 * @param set_component - the function to call to set a component
 * on the EC table.
 * @param ec - the object to pass as first argument to the above
 * functions.
 * @param plugin_notification - the function to call to emit a
 * notification.
 * @param plugin_log - the function to print a log message.
 */
struct ecsys *ecsys_new_(const tal_t *ctx,
			 bool (*get_component)(const void *ec,
					       const char **buffer,
					       const jsmntok_t **toks,
					       u32 entity,
					       const char *component),
			 void (*set_component)(void *ec,
					       u32 entity,
					       const char *component,
					       const char *buffer,
					       const jsmntok_t *tok),
			 void *ec,
			 void (*plugin_notification)(struct plugin *,
				 		     const char *method,
						     const char *buffer,
						     const jsmntok_t *tok),
			 void (*plugin_log)(struct plugin *,
					    enum log_level,
					    const char *));
#define ecsys_new(ctx, getc, setc, ec, notif, log) \
	ecsys_new_((ctx), \
		   typesafe_cb_postargs(void, const void *, (getc), (ec), \
					const char **, \
					const jsmntok_t **, \
					u32, \
					const char*), \
		   typesafe_cb_postargs(void, void *, (setc), (ec), \
					u32, \
					const char *, \
					const char *, \
					const jsmntok_t *), \
		   (ec), (notif), (log))

/** ecsys_register
 *
 * @brief Adds the specified function as the code for a specific
 * specified system.
 *
 * @desc This function is not idempotent and you must not call it
 * multiple times with the same system name.
 *
 * @param ecsys - the system handler to register into.
 * @param system - the name of the system.
 * @param requiredComponents - an array of strings.
 * The system is considered as matching entnties only if the
 * entity has all the specified components.
 * The array and its strings will be copied.
 * May be NULL, in which case this system will never actually
 * be matched (e.g. it is a marker system, not a real one).
 * @param numRequiredComponents - the length of the above
 * array.
 * @param disallowedComponents - an array of strings.
 * The sytem is considered as matching entities only if the
 * entity has none of the specified components.
 * The array and its strings will be copied.
 * May be NULL to indicate no disallowed components.
 * @param numDisallowedComponents - the length of the above
 * array.
 */
void ecsys_register(struct ecsys *ecsys,
		    const char *system,
		    const char *const *requiredComponents,
		    size_t numRequiredComponents,
		    const char *const *disallowedComponents,
		    size_t numDisallowedComponents);

/** ecsys_advance
 *
 * @brief Looks up the `lightningd:systems` component of the
 * given entity and searches for matching systems, updates
 * the `lightningd:systems`, then triggers the matched
 * system.
 *
 * @param plugin - the plugin this is running in.
 * @param ecsys - the system handler to trigger.
 * @param entity - the entity number to advance.
 * @param cb - the function to call in case of success.
 * @param errcb - the function to call in case of failure.
 * @param cbarg - the object to pass to the above functions.
 *
 * @desc The callback will be called after `lightningd:systems`
 * has been updated, but before the system code starts
 * executing.
 */
struct command_result *ecsys_advance_(struct plugin *plugin,
				      struct ecsys *ecsys,
				      u32 entity,
				      struct command_result *(*cb)(struct plugin *,
								   struct ecsys *,
								   void *cbarg),
				      struct command_result *(*errcb)(struct plugin *,
								      struct ecsys *,
								      errcode_t,
								      void *cbarg),
				      void *cbarg);
#define ecsys_advance(plugin, sys, e, cb, errcb, cbarg) \
	ecsys_advance_((plugin), (sys), (e), \
		       typesafe_cb_preargs(struct command_result *, void*, \
					   (cb), (cbarg), \
					   struct plugin *, \
					   struct ecsys *), \
		       typesafe_cb_preargs(struct command_result *, void*, \
					   (errcb), (cbarg), \
					   struct plugin *, \
					   struct ecsys *, \
					   errcode_t), \
		       (cbarg))

/** ecsys_advance_done
 *
 * @brief Advances, then completes execution of the
 * system.
 * Should only be called from system code.
 *
 * @param plugin - the plugin this is running in.
 * @param ecsys - the system handler to trigger.
 * @param entity - the entity number to advance.
 *
 * @desc We expect a majority of systems will just
 * advance at the end of their processing and not
 * want to have to handle failure of advance.
 * Failures are printed as error-level logs.
 */
void ecsys_advance_done(struct plugin *plugin,
			struct ecsys *ecsys,
			u32 entity);

/** ecsys_system_exists
 *
 * @brief Check if a system of a specific name is already
 * registered.
 *
 * @param ecsys - the system handler to query.
 * @param system - the name of the system to check.
 *
 * @return - true if a system of the given name is already
 * registered, false otherwise.
 */
bool ecsys_system_exists(const struct ecsys *ecsys,
			 const char *system);

/* The `lightningd:systems` component is not attached, or
 * does not have a valid `systems` field or a valid `current`
 * field, or one of the listed `systems` is not registered.  */
static const errcode_t PAY_ECS_INVALID_SYSTEMS_COMPONENT = 2200;
/* None of the systems listed matched.  */
static const errcode_t PAY_ECS_NOT_ADVANCEABLE = 2201;

#define ECSYS_SYSTEM_NOTIFICATION "payecs_system_invoke"

#endif /* LIGHTNING_PLUGINS_PAYZ_ECS_ECSYS_H */
