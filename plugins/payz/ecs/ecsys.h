#ifndef LIGHTNING_PLUGINS_PAYZ_ECS_ECSYS_H
#define LIGHTNING_PLUGINS_PAYZ_ECS_ECSYS_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<ccan/time/time.h>
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
 * dependencies (the plugin timer code, the EC table object, etc).
 * This is an application of the dependency inversion principle.
 *
 * @param ctx - the owner of this system handler.
 * @param get_component - the function to call to get a component
 * on the EC table.
 * @param set_component - the function to call to set a component
 * on the EC table.
 * @param ec - the object to pass as first argument to the above
 * functions.
 * @param plugin_timer - the function to call to add a new timer.
 * @param timer_complete - the function to call to complete
 * execution of a timer.
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
			 void *(*plugin_timer)(struct plugin *,
					       struct timerel t,
					       void (*cb)(void*),
					       void *cb_arg),
			 struct command_result *(*timer_complete)(struct plugin *),
			 void (*plugin_log)(struct plugin *,
					    enum log_level,
					    const char *));
#define ecsys_new(ctx, getc, setc, ec, timer, tcomp, log) \
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
		   (ec), (timer), (tcomp), (log))

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
 * @param code - the function to invoke when this system is
 * matched.
 * It is passed the plugin and the system handler.
 * It is also provided the entity number being triggered.
 * @param arg - the argument to pass to the above function.
 */
void ecsys_register_(struct ecsys *ecsys,
		     const char *system,
		     const char *const *requiredComponents,
		     size_t numRequiredComponents,
		     const char *const *disallowedComponents,
		     size_t numDisallowedComponents,
		     struct command_result *(*code)(struct plugin *plugin,
						    struct ecsys *ecsys,
						    u32 entity,
						    void *arg),
		     void *arg);
#define ecsys_register(ecsys_o, sys, req, nreq, dis, ndis, code, arg) \
	ecsys_register_((ecsys_o), (sys), (req), (nreq), (dis), (ndis), \
			typesafe_cb_preargs(struct command_result *,\
					    void *, (code), (arg), \
					    struct plugin *, \
					    struct ecsys *, \
					    u32), \
			(arg))

/** ecsys_system_done
 *
 * @brief Signals that a system has completed execution.
 *
 * @param plugin - the plugin passed to the system code.
 * @param ecsys - the system handler passed to the system code.
 *
 * @return Return this from the system code to signal that the
 * system has completed execution.
 */
struct command_result *ecsys_system_done(struct plugin *plugin,
					 struct ecsys *ecsys);

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
struct command_result *ecsys_advance_done(struct plugin *plugin,
					  struct ecsys *ecsys,
					  u32 entity);

/* The `lightningd:systems` component is not attached, or
 * does not have a valid `systems` field or a valid `next`
 * field, or one of the listed `systems` is not registered.  */
static const errcode_t PAY_ECS_INVALID_SYSTEMS_COMPONENT = 2200;
/* None of the systems listed matched.  */
static const errcode_t PAY_ECS_NOT_ADVANCEABLE = 2201;

#endif /* LIGHTNING_PLUGINS_PAYZ_ECS_ECSYS_H */
