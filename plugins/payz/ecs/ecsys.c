#include"ecsys.h"
#include<assert.h>
#include<ccan/json_out/json_out.h>
#include<ccan/strmap/strmap.h>
#include<ccan/tal/str/str.h>
#include<common/status_levels.h>
#include<common/utils.h>
#include<errno.h>
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
	struct command_result *(*code)(struct plugin *plugin,
				       struct ecsys *ecsys,
				       u32 entity,
				       void *arg);
	void *arg;
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
	void *(*plugin_timer)(struct plugin *,
			      struct timerel,
			      void (*)(void*),
			      void *);
	struct command_result *(*timer_complete)(struct plugin *);
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
			 void *(*plugin_timer)(struct plugin *,
					       struct timerel t,
					       void (*cb)(void*),
					       void *cb_arg),
			 struct command_result *(*timer_complete)(struct plugin *),
			 void (*plugin_log)(struct plugin *,
					    enum log_level,
					    const char *))
{
	struct ecsys *ecsys = tal(ctx, struct ecsys);

	strmap_init(&ecsys->system_map);
	ecsys->get_component = get_component;
	ecsys->set_component = set_component;
	ecsys->ec = ec;
	ecsys->plugin_timer = plugin_timer;
	ecsys->timer_complete = timer_complete;
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
		     void *arg)
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
	sys->code = code;
	sys->arg = arg;

	errno = 0;
	strmap_add(&ecsys->system_map, sys->system, sys);
	assert(errno != EEXIST);
}

/*-----------------------------------------------------------------------------
Done
-----------------------------------------------------------------------------*/

struct command_result *ecsys_system_done(struct plugin *plugin,
					 struct ecsys *ecsys)
{
	return ecsys->timer_complete(plugin);
}

/*-----------------------------------------------------------------------------
Advance
-----------------------------------------------------------------------------*/

static bool system_matches(const struct ecsys *ecsys,
			   u32 entity,
			   const struct ecsys_registered *system);
static void set_systems_component(const struct ecsys *ecsys,
				  u32 entity,
				  char** systems,
				  unsigned int next);
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
	const char *buffer;
	const jsmntok_t *tok;
	const jsmntok_t *j_systems;
	const jsmntok_t *j_system;
	char **systems;
	size_t nsystems;
	const jsmntok_t *j_next;
	unsigned int next;

	unsigned int i;
	unsigned int index;

	struct ecsys_registered *system;
	bool found;

	/* Validate the lightningd:systems component.  */
	ecsys->get_component(ecsys->ec,
			     &buffer, &tok,
			     entity,
			     "lightningd:systems");
	if (tok->type != JSMN_OBJECT)
		return ecsys_advance_error(plugin, ecsys, entity,
					   errcb, cbarg,
					   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
					   "Invalid `lightningd:systems`: "
					   "%.*s",
					   json_tok_full_len(tok),
					   json_tok_full(buffer, tok));
	j_systems = json_get_member(buffer, tok, "systems");
	if (!j_systems)
		return ecsys_advance_error(plugin, ecsys, entity,
					   errcb, cbarg,
					   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
					   "Invalid `lightningd:systems`: "
					   "`systems` field not found: %.*s",
					   json_tok_full_len(tok),
					   json_tok_full(buffer, tok));
	if (j_systems->type != JSMN_ARRAY)
		return ecsys_advance_error(plugin, ecsys, entity,
					   errcb, cbarg,
					   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
					   "Invalid `lightningd:systems`: "
					   "`systems` field not an array: "
					   "%.*s",
					   json_tok_full_len(j_systems),
					   json_tok_full(buffer, j_systems));
	nsystems = j_systems->size;
	j_next = json_get_member(buffer, tok, "next");
	if (j_next) {
		if (!json_to_number(buffer, tok, &next))
			return ecsys_advance_error(plugin, ecsys, entity,
						   errcb, cbarg,
						   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
						   "Invalid `lightningd:systems`: "
						   "`next` field not an "
						   "unsigned int: %.*s",
						   json_tok_full_len(j_next),
						   json_tok_full(buffer, j_next));
	} else
		next = 0;

	/* Load the systems from JSON to C strings.  */
	systems = tal_arr(tmpctx, char *, 0);
	json_for_each_arr (i, j_system, j_systems) {
		if (j_system->type != JSMN_STRING)
			return ecsys_advance_error(plugin, ecsys, entity,
						   errcb, cbarg,
						   PAY_ECS_INVALID_SYSTEMS_COMPONENT,
						   "Invalid `lightningd:systems`: "
						   "`systems` array contains "
						   "non-string: %.*s",
						   json_tok_full_len(j_system),
						   json_tok_full(buffer, j_system));
		tal_arr_expand(&systems,
			       json_strdup(tmpctx, buffer, j_system));
	}

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
	next = (index + 1) % nsystems;
	set_systems_component(ecsys, entity, systems, next);

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
static void set_systems_component(const struct ecsys *ecsys,
				  u32 entity,
				  char** systems,
				  unsigned int next)
{
	struct json_out *jout = json_out_new(tmpctx);
	const char *buffer;
	jsmntok_t *tok;

	size_t i;
	size_t len;

	json_out_start(jout, NULL, '{');
	json_out_start(jout, "systems", '[');
	for (i = 0; i < tal_count(systems); ++i)
		json_out_addstr(jout, NULL, systems[i]);
	json_out_end(jout, ']');
	json_out_add(jout, "next", false, "%u", next);
	json_out_end(jout, '}');
	json_out_finished(jout);

	buffer = json_out_contents(jout, &len);
	tok = json_parse_simple(tmpctx, buffer, len);

	ecsys->set_component(ecsys->ec, entity, "lightningd:systems",
			     buffer, tok);
}
struct ecsys_run_system {
	struct plugin *plugin;
	struct ecsys *ecsys;
	struct ecsys_registered *system;
	u32 entity;
};
static void run_system_cb(void *vinfo);
static void run_system(struct plugin *plugin,
		       struct ecsys *ecsys,
		       u32 entity,
		       struct ecsys_registered *system)
{
	struct ecsys_run_system *info;

	info = tal(plugin, struct ecsys_run_system);
	info->plugin = plugin;
	info->ecsys = ecsys;
	info->system = system;
	info->entity = entity;

	(void) ecsys->plugin_timer(plugin, time_from_sec(0),
				   &run_system_cb, (void *) info);
}
static void run_system_cb(void *vinfo)
{
	struct ecsys_run_system *info;
	info = (struct ecsys_run_system *) vinfo;

	struct plugin *plugin = info->plugin;
	struct ecsys *ecsys = info->ecsys;
	struct ecsys_registered *system = info->system;
	u32 entity = info->entity;
	tal_free(info);

	(void) system->code(plugin, ecsys, entity, system->arg);
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

	size_t i;
	size_t len;
	const jsmntok_t *key;
	const jsmntok_t *value;

	/* Generate message. */
	va_start(ap, fmt);
	msg = tal_vfmt(tmpctx, fmt, ap);
	va_end(ap);

	/* Fetch the existing `lightningd:systems` component.  */
	ecsys->get_component(ecsys->ec, &buffer, &toks,
			     entity, "lightningd:systems");

	/* Start building the object we will replace into
	 * `lightningd:systems`.  */
	json_out_start(jout, NULL, '{');
	/* If the current value is an object, add its contents verbatim.  */
	if (toks->type == JSMN_OBJECT) {
		json_for_each_obj (i, key, toks) {
			value = key + 1;
			/* Skip any existing `error` field.  */
			if (json_tok_streq(buffer, key, "error"))
				continue;

			/* Copy the value directly.  */
			memcpy(json_out_member_direct(jout,
						      json_strdup(tmpctx,
							          buffer,
								  key),
						      value->end - value->start),
			       buffer + value->start,
			       value->end - value->start);
		}
	}
	/* Now add a new `error` sub-object.  */
	json_out_start(jout, "error", '{');
	json_out_add(jout, "code", false, "%"PRIerrcode, code);
	json_out_add(jout, "message", true, "%s", msg);
	json_out_end(jout, '}');
	json_out_end(jout, '}');

	/* *FINALLY* put the jout into the `lightningd:systems`.  */
	buffer = json_out_contents(jout, &len);
	toks = json_parse_simple(tmpctx, buffer, len);
	ecsys->set_component(ecsys->ec, entity, "lightningd:systems",
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

struct command_result *ecsys_advance_done(struct plugin *plugin,
					  struct ecsys *ecsys,
					  u32 entity)
{
	struct ecsys_advance_done_info *inf;

	inf = tal(ecsys, struct ecsys_advance_done_info);
	inf->entity = entity;

	return ecsys_advance(plugin, ecsys, entity,
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
	return ecsys_system_done(plugin, ecsys);
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
