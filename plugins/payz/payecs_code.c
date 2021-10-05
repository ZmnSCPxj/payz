#include"payecs_code.h"
#include<assert.h>
#include<ccan/array_size/array_size.h>
#include<ccan/compiler/compiler.h>
#include<ccan/json_out/json_out.h>
#include<ccan/likely/likely.h>
#include<ccan/list/list.h>
#include<ccan/str/str.h>
#include<ccan/tal/str/str.h>
#include<ccan/strmap/strmap.h>
#include<ccan/take/take.h>
#include<ccan/time/time.h>
#include<common/json_stream.h>
#include<common/json_tok.h>
#include<common/jsonrpc_errors.h>
#include<common/param.h>
#include<plugins/payz/ecs/ecs.h>
#include<plugins/payz/parsing.h>
#include<plugins/payz/setsystems.h>
#include<plugins/payz/top.h>
#include<string.h>
#include<time.h>

/*-----------------------------------------------------------------------------
Commands
-----------------------------------------------------------------------------*/

static struct command_result *
payecs_newsystem(struct command *cmd,
		 const char *buf,
		 const jsmntok_t *params);
static struct command_result *
payecs_advance(struct command *cmd,
	       const char *buf,
	       const jsmntok_t *params);
static struct command_result *
payecs_getdefaultsystems(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *params);
static struct command_result *
payecs_setdefaultsystems(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *params);
static struct command_result *
payecs_systrace(struct command *cmd,
		const char *buf,
		const jsmntok_t *params);

static void
payecs_system_notification(struct command *cmd,
			   const char *buf,
			   const jsmntok_t *params);

const struct plugin_command payecs_code_commands[] = {
	{
		"payecs_newsystem",
		"payment",
		"Register a new {system}, which will invoke {command} if "
		"it is used in an entity with all {required} components and "
		"with none of the {disallowed} components.",
		"Register new system.",
		&payecs_newsystem
	},
	{
		"payecs_advance",
		"payment",
		"Advance the processing of systems of the given {entity}.",
		"Advance entity processing.",
		&payecs_advance
	},
	{
		"payecs_getdefaultsystems",
		"payment",
		"Return the list of built-in systems for normal payment flow.",
		"Return the list of built-in systems for normal payment flow.",
		&payecs_getdefaultsystems
	},
	{
		"payecs_setdefaultsystems",
		"payment",
		"Set the given {entity} to use the built-in systmes for "
		"normal payment flow, optionally adding {prepend}ed and "
		"{append}ed systems.",
		"Set entity to use normal payment flow, possibly with "
		"additional systems.",
		&payecs_setdefaultsystems
	},
	{
		"payecs_systrace",
		"payment",
		"Return a trace of systems that were triggered for a "
		"specified {entity}.",
		"Trace the systems that ran on the given entity.",
		&payecs_systrace
	}
};
const size_t num_payecs_code_commands = ARRAY_SIZE(payecs_code_commands);

const struct plugin_notification payecs_code_notifications[] = {
	{
		ECS_SYSTEM_NOTIFICATION,
		&payecs_system_notification
	}
};
const size_t num_payecs_code_notifications = ARRAY_SIZE(payecs_code_notifications);

const char *payecs_code_topics[] = {
	ECS_SYSTEM_NOTIFICATION
};
const size_t num_payecs_code_topics = ARRAY_SIZE(payecs_code_topics);

/*-----------------------------------------------------------------------------
Registry of external systems
-----------------------------------------------------------------------------*/

/*~
 * The payecs system includes several built-in systems, but an
 * advantage of the payecs system is that it allows plugins to
 * also add their own systems, which can then be triggered for
 * specific payments by simply adding the systems to the
 * `lightningd:systems` component.
 *
 * This part of the code handles this registry of external systems.
 */

/** struct payecs_external_system
 *
 * @brief Represents a registered external system.
 */
struct payecs_external_system {
	/* The name of the system.  */
	const char *system;
	/* The components required by the system.
	 * We will pass in these components to the system (to
	 * reduce RPC-call overhead for components the system
	 * is likely to want to look up anyway), so store them
	 * here.
	 */
	const char **required;
	/* The components disallowed for matching the
	 * system.
	 * While these are not passed to the system code, we
	 * still need to store it here so that we can compare
	 * subsequent calls to `payecs_newsystem` as idempotent.
	 */
	const char **disallowed;
};

static bool payecs_registry_initialized = false;
static STRMAP(struct payecs_external_system *) payecs_registry;

static void payecs_registry_init_if_needed(void)
{
	if (likely(payecs_registry_initialized))
		return;

	strmap_init(&payecs_registry);
	payecs_registry_initialized = true;
}

/** payecs_register
 *
 * @brief Add an entry to the registry.
 *
 * @param system - the name of the system.
 * @param required - an array of component names that are
 * required for this system to trigger.
 * @param disallowed - an array of component names that
 * must not exist for this system to trigger.
 *
 * @return - True if registration was OK (system does not
 * exist, or system exists but has exactly the same
 * parameters).
 * False if the system already exists and the parameters
 * are different.
 */
static bool payecs_register(const char *system TAKES,
			    const char **required TAKES,
			    const char **disallowed TAKES)
{
	struct payecs_external_system *exsys;

	bool ok = true;

	size_t i;
	struct ecs_register_desc *reg;

	/* First look it up in the strmap.  */
	exsys = strmap_get(&payecs_registry, system);
	if (exsys) {
		/* Check it is the same arguments.  */
		ok = ok && streq(system, exsys->system);
		ok = ok &&
		     (tal_count(required) == tal_count(exsys->required));
		ok = ok &&
		     (tal_count(disallowed) == tal_count(exsys->disallowed));
		for (i = 0; ok && i < tal_count(required); ++i)
			ok = streq(required[i], exsys->required[i]);
		for (i = 0; ok && i < tal_count(disallowed); ++i)
			ok = streq(disallowed[i], exsys->disallowed[i]);

		/* Regardless of result, we will not use the
		 * arguments, so free them if taken.  */
		if (taken(system))
			tal_free(system);
		if (taken(required))
			tal_free(required);
		if (taken(disallowed))
			tal_free(disallowed);

		return ok;
	}

	/* Then check the ECS for a system of the same name.
	 * If there is one, then it is a builtin system.
	 */
	if (ecs_system_exists(payz_top->ecs, system)) {
		/* Fail.
		 * Free taken arguments.  */
		if (taken(system))
			tal_free(system);
		if (taken(required))
			tal_free(required);
		if (taken(disallowed))
			tal_free(disallowed);

		return false;
	}

	/* Create the entry.  */
	exsys = tal(payz_top, struct payecs_external_system);
	exsys->system = tal_strdup(exsys, system);
	if (taken(required))
		exsys->required = tal_steal(exsys, required);
	else {
		exsys->required = tal_arr(exsys, const char *,
					  tal_count(required));
		for (i = 0; i < tal_count(required); ++i)
			exsys->required[i] = tal_strdup(exsys,
							required[i]);
	}
	if (taken(disallowed))
		exsys->disallowed = tal_steal(exsys, disallowed);
	else {
		exsys->disallowed = tal_arr(exsys, const char *,
					    tal_count(disallowed));
		for (i = 0; i < tal_count(disallowed); ++i)
			exsys->disallowed[i] = tal_strdup(exsys,
							  disallowed[i]);
	}

	/* Add it to our externals registry.  */
	payecs_registry_init_if_needed();
	strmap_add(&payecs_registry, exsys->system, exsys);
	/* Now hand it to the actual ECS.  */
	reg = ecs_register_begin(tmpctx);
	ecs_register_name(&reg, exsys->system);
	for (i = 0; i < tal_count(exsys->required); ++i)
		ecs_register_require(&reg, exsys->required[i]);
	for (i = 0; i < tal_count(exsys->disallowed); ++i)
		ecs_register_disallow(&reg, exsys->disallowed[i]);
	ecs_register_done(&reg);
	ecs_register(payz_top->ecs, take(reg));

	return true;
}

/*-----------------------------------------------------------------------------
System Registration Command
-----------------------------------------------------------------------------*/

static struct command_result *
payecs_newsystem(struct command *cmd,
		 const char *buf,
		 const jsmntok_t *params)
{
	const char *system;
	const char **required;
	const char **disallowed;

	if (!param(cmd, buf, params,
		   p_req("system", &param_string, &system),
		   p_req("required", &param_array_of_strings,
			 &required),
		   p_opt("disallowed", &param_array_of_strings,
			 &disallowed),
		   NULL))
		return command_param_failed();

	if (!payecs_register(system, take(required), take(disallowed)))
		return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
				    "Conflict with existing `system`: %s",
				    system);

	/* Return empty object.  */
	return command_success(cmd, json_out_obj(cmd, NULL, NULL));
}

/*-----------------------------------------------------------------------------
Entity processing advancement
-----------------------------------------------------------------------------*/

/** struct payecs_advance_closure
 *
 * @brief Contains information about the advance command given.
 */
struct payecs_advance_closure {
	struct command *cmd;
	u32 entity;
};

static struct command_result *
payecs_advance_ok(struct plugin *, struct ecs *,
		  struct payecs_advance_closure *);
static struct command_result *
payecs_advance_ng(struct plugin *, struct ecs *,
		  errcode_t errcode,
		  struct payecs_advance_closure *);

static struct command_result *
payecs_advance(struct command *cmd,
	       const char *buf,
	       const jsmntok_t *params)
{
	unsigned int *entity;

	struct payecs_advance_closure *closure;

	if (!param(cmd, buf, params,
		   p_req("entity", &param_number, &entity),
		   NULL))
		return command_param_failed();

	/* Store information in closure.  */
	closure = tal(cmd, struct payecs_advance_closure);
	closure->cmd = cmd;
	closure->entity = (u32) *entity;

	return ecs_advance((cmd->plugin), payz_top->ecs, (u32) *entity,
			   &payecs_advance_ok,
			   &payecs_advance_ng,
			   closure);
}

static struct command_result *
payecs_advance_ok(struct plugin *plugin UNUSED, struct ecs *ecs UNUSED,
		  struct payecs_advance_closure *closure)
{
	/* Normal exit.  */
	struct command *cmd = closure->cmd;

	tal_steal(tmpctx, closure);

	return command_success(cmd, json_out_obj(cmd, NULL, NULL));
}

static struct command_result *
payecs_advance_ng(struct plugin *plugin UNUSED, struct ecs *ecs,
		  errcode_t errcode,
		  struct payecs_advance_closure *closure)
{
	/* Fail!  */
	struct command *cmd = closure->cmd;
	u32 entity = closure->entity;

	const char *compbuf;
	const jsmntok_t *comptok;
	const char *message;

	const char *scan_err;

	tal_steal(tmpctx, closure);

	/* Extract message from `lightningd:systems` component.  */
	ecs_get_component(ecs, &compbuf, &comptok,
			  entity, "lightningd:systems");
	scan_err = json_scan(tmpctx, compbuf, comptok,
			     "{error:{message:%}}",
			     JSON_SCAN_TAL(tmpctx, json_strdup, &message));
	assert(!scan_err);

	return command_fail(cmd, errcode, "%s", message);
}

/*-----------------------------------------------------------------------------
Default Systems
-----------------------------------------------------------------------------*/

/** json_splice_default_systems
 *
 * @brief Common function to splice the default systems into an
 * array of strings.
 *
 * @param js - the JSON stream to extend, must currently have an
 * array begun.
 */
static void json_splice_default_systems(struct json_stream *js)
{
	size_t i;
	for (i = 0; i < tal_count(payz_top->default_systems); ++i)
		json_add_string(js, NULL,
				payz_top->default_systems[i]);
}


static struct command_result *
payecs_getdefaultsystems(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *params)
{
	struct json_stream *out;

	if (!param(cmd, buf, params, NULL))
		return command_param_failed();

	out = jsonrpc_stream_success(cmd);
	json_array_start(out, "systems");
	json_splice_default_systems(out);
	json_array_end(out);
	return command_finished(cmd, out);
}

static struct command_result *
payecs_setdefaultsystems(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *params)
{
	unsigned int *entity;
	const char **prepend;
	const char **append;

	size_t i;

	struct json_stream *js;
	const char *newcompbuf;
	size_t newcomplen;
	jsmntok_t *newcomptok;

	if (!param(cmd, buf, params,
		   p_req("entity", &param_number, &entity),
		   p_opt("prepend", &param_array_of_strings, &prepend),
		   p_opt("append", &param_array_of_strings, &append),
		   NULL))
		return command_param_failed();

	/* Validate the prepended and appended systems.  */
	for (i = 0; i < tal_count(prepend); ++i)
		if (!ecs_system_exists(payz_top->ecs, prepend[i]))
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "Unregistered system: %s",
					    prepend[i]);
	for (i = 0; i < tal_count(append); ++i)
		if (!ecs_system_exists(payz_top->ecs, append[i]))
			return command_fail(cmd, JSONRPC2_INVALID_PARAMS,
					    "Unregistered system: %s",
					    append[i]);

	/* Now create the new component.  */
	js = new_json_stream(tmpctx, NULL, NULL);
	json_array_start(js, NULL);
	for (i = 0; i < tal_count(prepend); ++i)
		json_add_string(js, NULL, prepend[i]);
	json_splice_default_systems(js);
	for (i = 0; i < tal_count(append); ++i)
		json_add_string(js, NULL, append[i]);
	json_array_end(js);

	/* Set the value.  */
	newcompbuf = json_out_contents(js->jout, &newcomplen);
	newcomptok = json_parse_simple(tmpctx, newcompbuf, newcomplen);
	payz_setsystems_tok(*entity, "systems", newcompbuf, newcomptok);

	/* Return empty object.  */
	return command_success(cmd, json_out_obj(cmd, NULL, NULL));
}

/*-----------------------------------------------------------------------------
Systrace
-----------------------------------------------------------------------------*/

/*~ The systrace facility allows plugin developers to have a view of
 * the operations of the payment system by tracing what systems were
 * triggered on entities.
 *
 * This is just a trace of the most recent `payecs_system_invoke`
 * notifications, recording the `params` given.
 */

/** struct payecs_systrace_entry
 *
 * @brief Represents an entry into the global systrace entry.
 */
struct payecs_systrace_entry {
	struct list_node list;

	/** The entity this was triggered on.  */
	u32 entity;

	/** The JSON 'system' and `entity` we got from the RPC
	 * notification, plus a 'time' field.
	 * This is a nul-terminated C string containing formatted
	 * JSON text.
	 */
	char *json;
};

/** payecs_systraces_remaining
 *
 * @brief The number of systraces we will accept until we start
 * removing old entries.
 */
static size_t payecs_systraces_remaining = 100000;
/** systraces
 *
 * @brief The actual list of systraces.
 */
static LIST_HEAD(payecs_systraces);

static char *format_time(const tal_t *ctx, struct timeabs time);

static void payecs_systrace_add(struct plugin *plugin,
				const char *buf,
				const jsmntok_t *params)
{
	struct payecs_systrace_entry *entry;

	u32 entity;

	const char *error;

	const jsmntok_t *system;
	const jsmntok_t *entity_obj;

	/* Check if we can get the entity ID.  */
	error = json_scan(tmpctx, buf, params,
			  "{entity:{entity:%}}",
			  JSON_SCAN(json_to_u32, &entity));
	if (error) {
		plugin_log(plugin, LOG_UNUSUAL,
			   "Invalid '%s' parameters: %s: %.*s",
			   ECS_SYSTEM_NOTIFICATION, error,
			   json_tok_full_len(params),
			   json_tok_full(buf, params));
		return;
	}

	/* If the list is too long already, erase old entries.  */
	if (payecs_systraces_remaining == 0) {
		tal_free(list_pop(&payecs_systraces,
				  struct payecs_systrace_entry,
				  list));
		++payecs_systraces_remaining;
	}

	/* Extract these.  */
	system = json_get_member(buf, params, "system");
	entity_obj = json_get_member(buf, params, "entity");
	if (!system || !entity_obj) {
		plugin_log(plugin, LOG_UNUSUAL,
			   "'%s' parameter missing 'system' or 'entity': %.*s",
			   ECS_SYSTEM_NOTIFICATION,
			   json_tok_full_len(params),
			   json_tok_full(buf, params));
		return;
	}

	entry = tal(payz_top, struct payecs_systrace_entry);
	entry->entity = entity;
	entry->json = tal_fmt(entry,
			      "{\"time\": \"%s\", \"system\": %.*s,"
			      " \"entity\": %.*s}",
			      format_time(tmpctx, time_now()),
			      json_tok_full_len(system),
			      json_tok_full(buf, system),
			      json_tok_full_len(entity_obj),
			      json_tok_full(buf, entity_obj));
	list_add_tail(&payecs_systraces, &entry->list);
	--payecs_systraces_remaining;
}

static struct command_result *
payecs_systrace(struct command *cmd,
		const char *buf,
		const jsmntok_t *params)
{
	unsigned int *entity;

	struct json_stream *out;
	struct payecs_systrace_entry *entry;

	if (!param(cmd, buf, params,
		   p_req("entity", &param_number, &entity),
		   NULL))
		return command_param_failed();

	out = jsonrpc_stream_success(cmd);
	json_add_u32(out, "entity", (u32) *entity);
	json_array_start(out, "trace");
	list_for_each (&payecs_systraces, entry, list) {
		if (entry->entity != *entity)
			continue;
		json_add_literal(out, NULL,
				 entry->json, strlen(entry->json));
	}
	json_array_end(out);
	return command_finished(cmd, out);
}

static char *format_time(const tal_t *ctx, struct timeabs time)
{
	char iso8601[sizeof("YYYY-mm-ddTHH:MM:SS")];

	strftime(iso8601, sizeof(iso8601),
		 "%FT%T", gmtime(&time.ts.tv_sec));
	return tal_fmt(ctx, "%s.%03dZ",
		       iso8601,
		       (int) time.ts.tv_nsec / 1000000);
}

/*-----------------------------------------------------------------------------
ECS System Trigger Notification
-----------------------------------------------------------------------------*/

static void
payecs_system_notification(struct command *cmd,
			   const char *buf,
			   const jsmntok_t *params)
{
	const jsmntok_t *payload;

	payload = json_get_member(buf, params, "payload");
	if (!payload) {
		plugin_log(cmd->plugin, LOG_UNUSUAL,
			   "%s parameters has no payload: %.*s",
			   ECS_SYSTEM_NOTIFICATION,
			   json_tok_full_len(params),
			   json_tok_full(buf, params));
		return;
	}
	plugin_log(cmd->plugin, LOG_DBG,
		   "%s payload: %.*s",
		   ECS_SYSTEM_NOTIFICATION,
		   json_tok_full_len(payload),
		   json_tok_full(buf, payload));
	payecs_systrace_add(cmd->plugin, buf, payload);
	ecs_system_notify(payz_top->ecs, cmd, buf, payload);
}
