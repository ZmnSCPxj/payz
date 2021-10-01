#include"ecsys.h"
#include<assert.h>
#include<ccan/json_out/json_out.h>
#include<ccan/strmap/strmap.h>
#include<ccan/tal/str/str.h>
#include<common/json_stream.h>
#include<common/status_levels.h>
#include<common/utils.h>
#include<errno.h>
#include<plugins/libplugin.h>
#include<plugins/payz/parsing.h>
#include<plugins/payz/setsystems.h>
#include<stdarg.h>

/*-----------------------------------------------------------------------------
Objects
-----------------------------------------------------------------------------*/

/** struct ecsys_registered
 *
 * @brief Represents a registered system.
 */
struct ecsys_registered {
	const char *system;
	char **requiredComponents;
	char **disallowedComponents;
};

struct ecsys {
	STRMAP(struct ecsys_registered *) system_map;

	bool (*get_component)(const void *ec,
			      const char **,
			      const jsmntok_t **,
			      u32,
			      const char *);
	void (*set_component)(void *ec,
			      u32,
			      const char *,
			      const char *,
			      const jsmntok_t *);
	void *ec;
	struct json_stream *(*plugin_notification_start)(struct plugin *,
							 const char *method);
	void (*plugin_notification_end)(struct plugin *,
					struct json_stream *stream);
	void (*plugin_log)(struct plugin *,
			   enum log_level,
			   const char *);
};

/*-----------------------------------------------------------------------------
Construction
-----------------------------------------------------------------------------*/

static void ecsys_destroy(struct ecsys *ecsys);
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
			 struct json_stream *(*plugin_notification_start)(struct plugin *,
									  const char *method),
			 void (*plugin_notification_end)(struct plugin *,
				 			 struct json_stream *stream),
			 void (*plugin_log)(struct plugin *,
					    enum log_level,
					    const char *))
{
	struct ecsys *ecsys = tal(ctx, struct ecsys);

	strmap_init(&ecsys->system_map);
	ecsys->get_component = get_component;
	ecsys->set_component = set_component;
	ecsys->ec = ec;
	ecsys->plugin_notification_start = plugin_notification_start;
	ecsys->plugin_notification_end = plugin_notification_end;
	ecsys->plugin_log = plugin_log;

	tal_add_destructor(ecsys, &ecsys_destroy);

	return ecsys;
}
static void ecsys_destroy(struct ecsys *ecsys)
{
	/* strmap uses malloc, so clear it to ensure everything
	 * it uses is freed.
	 */
	strmap_clear(&ecsys->system_map);
}

/*-----------------------------------------------------------------------------
Registration
-----------------------------------------------------------------------------*/

void ecsys_register(struct ecsys *ecsys,
		    const char *system,
		    const char *const *requiredComponents,
		    size_t numRequiredComponents,
		    const char *const *disallowedComponents,
		    size_t numDisallowedComponents)
{
	struct ecsys_registered *sys;
	size_t i;

	if (!requiredComponents)
		numRequiredComponents = 0;
	if (!disallowedComponents)
		numDisallowedComponents = 0;

	sys = tal(ecsys, struct ecsys_registered);
	sys->system = tal_strdup(sys, system);
	sys->requiredComponents = tal_arr(sys, char*,
					  numRequiredComponents);
	for (i = 0; i < numRequiredComponents; ++i)
		sys->requiredComponents[i] =
			tal_strdup(sys, requiredComponents[i]);
	sys->disallowedComponents = tal_arr(sys, char*,
					    numDisallowedComponents);
	for (i = 0; i < numDisallowedComponents; ++i)
		sys->disallowedComponents[i] =
			tal_strdup(sys, disallowedComponents[i]);

	errno = 0;
	strmap_add(&ecsys->system_map, sys->system, sys);
	assert(errno != EEXIST);
}

/*-----------------------------------------------------------------------------
Advance
-----------------------------------------------------------------------------*/

static bool system_matches(const struct ecsys *ecsys,
			   u32 entity,
			   const struct ecsys_registered *system);
static void run_system(struct plugin *plugin,
		       struct ecsys *ecsys,
		       u32 entity,
		       struct ecsys_registered *system);
/* Call to add an `error` field to `lightningd:systems`.  */
static struct command_result *
PRINTF_FMT(7, 8)
ecsys_advance_error(struct plugin *plugin,
		    struct ecsys *ecsys,
		    u32 entity,
		    struct command_result *(*errcb)(struct plugin *,
						    struct ecsys *,
						    errcode_t,
						    void *cbarg),
		    void *cbarg,
		    errcode_t code,
		    const char *fmt, ...);

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
				      void *cbarg)
{
	const char **systems;
	size_t nsystems;
	unsigned int current;
	unsigned int next;

	unsigned int i;
	unsigned int index;

	struct ecsys_registered *system;
	bool found;

	char *buffer;
	jsmntok_t *toks;

	/* Validate the lightningd:systems component.  */
	found = payz_generic_getsystems_tal(tmpctx,
					    ecsys->get_component, ecsys->ec,
					    entity, "systems",
					    &json_to_array_of_strings,
					    &systems);
	if (!found)
		return ecsys_advance_error(plugin, ecsys, entity,
					   errcb, cbarg,
					   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
					   "Invalid `lightningd:systems`: "
					   "invalid or absent `systems` field.");
	nsystems = tal_count(systems);

	found = payz_generic_getsystems(ecsys->get_component, ecsys->ec,
					entity, "current",
					&json_to_number, &current);
	if (found)
		next = current + 1;
	else
		next = 0;

	/* Search for matching system.  */
	found = false;
	for (i = 0, index = next;
	     i < nsystems;
	     ++i, index = (index + 1) % nsystems) {
		/* Find the system.  */
		system = strmap_get(&ecsys->system_map, systems[index]);
		if (!system)
			return ecsys_advance_error(plugin, ecsys, entity,
						   errcb, cbarg,
						   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
						   "Invalid `lightningd:systems`: "
						   "`systems` array contains "
						   "unregistered system: %s",
						   systems[index]);

		if (system_matches(ecsys, entity, system)) {
			found = true;
			break;
		}
	}
	if (!found)
		return ecsys_advance_error(plugin, ecsys, entity,
					   errcb, cbarg,
					   PAY_ECS_NOT_ADVANCEABLE,
					   "No systems match, cannot advance.");

	/* Update component.  */
	current = index;
	buffer = tal_fmt(tmpctx, "%u", current);
	toks = json_parse_simple(tmpctx, buffer, strlen(buffer));
	payz_generic_setsystems_tok(ecsys->get_component,
				    ecsys->set_component,
				    ecsys->ec,
				    entity, "current",
				    buffer, toks);

	/* Trigger execution.  */
	run_system(plugin, ecsys, entity, system);

	/* Normal exit.  */
	return cb(plugin, ecsys, cbarg);
}

static bool system_matches(const struct ecsys *ecsys,
			   u32 entity,
			   const struct ecsys_registered *system)
{
	const char *buffer;
	const jsmntok_t *tok;

	const char *component;
	size_t i;

	/* Systems with an empty requiredComponents never match
	 * anything.  */
	if (tal_count(system->requiredComponents) == 0)
		return false;

	for (i = 0; i < tal_count(system->requiredComponents); ++i) {
		component = system->requiredComponents[i];
		if (!ecsys->get_component(ecsys->ec, &buffer, &tok,
					  entity, component))
			return false;
	}

	for (i = 0; i < tal_count(system->disallowedComponents); ++i) {
		component = system->disallowedComponents[i];
		if (ecsys->get_component(ecsys->ec, &buffer, &tok,
					 entity, component))
			return false;
	}

	return true;
}

static void run_system(struct plugin *plugin,
		       struct ecsys *ecsys,
		       u32 entity,
		       struct ecsys_registered *system)
{
	struct json_stream *js;

	unsigned int i;

	/* Construct params.  */
	js = ecsys->plugin_notification_start(plugin, ECSYS_SYSTEM_NOTIFICATION);
	json_add_string(js, "system", system->system);

	/* Construct entity, pass in the components that are
	 * required by the system.
	 */
	json_object_start(js, "entity");
	json_add_u32(js, "entity", entity);
	for (i = 0; i < tal_count(system->requiredComponents); ++i) {
		const char *component = system->requiredComponents[i];
		const char *cmpbuf;
		const jsmntok_t *cmptok;

		(void) ecsys->get_component(ecsys->ec,
					    &cmpbuf, &cmptok,
					    entity, component);
		json_add_tok(js, component, cmptok, cmpbuf);
	}
	json_object_end(js);

	/* Now raise the notification.  */
	ecsys->plugin_notification_end(plugin, js);
}

static struct command_result *
PRINTF_FMT(7, 8)
ecsys_advance_error(struct plugin *plugin,
		    struct ecsys *ecsys,
		    u32 entity,
		    struct command_result *(*errcb)(struct plugin *,
						    struct ecsys *,
						    errcode_t,
						    void *cbarg),
		    void *cbarg,
		    errcode_t code,
		    const char *fmt, ...)
{
	const char *msg;
	va_list ap;

	const char *buffer;
	const jsmntok_t *toks;

	struct json_out *jout = json_out_new(tmpctx);

	size_t len;

	/* Generate message. */
	va_start(ap, fmt);
	msg = tal_vfmt(tmpctx, fmt, ap);
	va_end(ap);

	/* Generate `error` sub-object.  */
	json_out_start(jout, NULL, '{');
	json_out_add(jout, "code", false, "%"PRIerrcode, code);
	json_out_add(jout, "message", true, "%s", msg);
	json_out_end(jout, '}');

	/* Put the jout into the `lightningd:systems`.  */
	buffer = json_out_contents(jout, &len);
	toks = json_parse_simple(tmpctx, buffer, len);
	payz_generic_setsystems_tok(ecsys->get_component,
				    ecsys->set_component,
				    ecsys->ec,
				    entity,
				    "error",
				    buffer, toks);

	/* Call back.  */
	return errcb(plugin, ecsys, code, cbarg);
}

/*-----------------------------------------------------------------------------
Advance Done
-----------------------------------------------------------------------------*/

struct ecsys_advance_done_info {
	u32 entity;
};

static struct command_result *
ecsys_done_wrap(struct plugin *plugin,
		struct ecsys *ecsys,
		struct ecsys_advance_done_info *inf);
static struct command_result *
ecsys_err_wrap(struct plugin *plugin,
	       struct ecsys *ecsys,
	       errcode_t err,
	       struct ecsys_advance_done_info *inf);

void ecsys_advance_done(struct plugin *plugin,
			struct ecsys *ecsys,
			u32 entity)
{
	struct ecsys_advance_done_info *inf;

	inf = tal(ecsys, struct ecsys_advance_done_info);
	inf->entity = entity;

	(void) ecsys_advance(plugin, ecsys, entity,
			     &ecsys_done_wrap,
			     &ecsys_err_wrap,
			     inf);
}
static struct command_result *
ecsys_done_wrap(struct plugin *plugin,
		struct ecsys *ecsys,
		struct ecsys_advance_done_info *inf)
{
	tal_free(inf);
	return command_still_pending(NULL);
}
static struct command_result *
ecsys_err_wrap(struct plugin *plugin,
	       struct ecsys *ecsys,
	       errcode_t code,
	       struct ecsys_advance_done_info *inf)
{
	u32 entity = inf->entity;
	enum log_level level;
	char *msg;

	const char *err;

	const char *buffer;
	const jsmntok_t *tok;

	if (code == PAY_ECS_NOT_ADVANCEABLE)
		level = LOG_UNUSUAL;
	else
		level = LOG_BROKEN;

	/* Extract the error.message.  */
	ecsys->get_component(ecsys->ec, &buffer, &tok,
			     entity, "lightningd:systems");
	err = json_scan(tmpctx, buffer, tok,
			"{error:{message:%}}",
			JSON_SCAN_TAL(tmpctx, json_strdup, &msg));
	assert(!err);
	(void)err;

	ecsys->plugin_log(plugin, level,
			  tal_fmt(tmpctx,
				 "entity %"PRIu32": %s",
				 entity, msg));

	return ecsys_done_wrap(plugin, ecsys, inf);
}

/*-----------------------------------------------------------------------------
Exist
-----------------------------------------------------------------------------*/

bool ecsys_system_exists(const struct ecsys *ecsys,
			 const char *system)
{
	return strmap_get(&ecsys->system_map, system) != NULL;
}
