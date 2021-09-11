#include"payecs_code.h"
#include<assert.h>
#include<ccan/array_size/array_size.h>
#include<ccan/compiler/compiler.h>
#include<ccan/json_out/json_out.h>
#include<ccan/likely/likely.h>
#include<ccan/str/str.h>
#include<ccan/tal/str/str.h>
#include<ccan/strmap/strmap.h>
#include<ccan/take/take.h>
#include<common/json_stream.h>
#include<common/json_tok.h>
#include<common/jsonrpc_errors.h>
#include<common/param.h>
#include<plugins/payz/ecs/ecs.h>
#include<plugins/payz/parsing.h>
#include<plugins/payz/top.h>

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

const struct plugin_command payecs_code_commands[] = {
	{
		"payecs_newsystem",
		"payment",
		"Register a new {system}, which will invoke {command} if "
		"it is used in an entity with all {required} components and "
		"wht none of the {disallowed} components.",
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
		"Set entity to use normal payment flow.",
		&payecs_setdefaultsystems
	}
};
const size_t num_payecs_code_commands = ARRAY_SIZE(payecs_code_commands);

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
	/* The command to invoke.  */
	const char *command;
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

static struct command_result *
payecs_external_proxy(struct plugin *, struct ecs *, const char *system,
		      u32 entity);

/** payecs_register
 *
 * @brief Add an entry to the registry.
 *
 * @param system - the name of the system.
 * @param command - the name of the command.
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
			    const char *command TAKES,
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
		if (taken(command))
			tal_free(command);
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
		if (taken(command))
			tal_free(command);
		if (taken(required))
			tal_free(required);
		if (taken(disallowed))
			tal_free(disallowed);

		return false;
	}

	/* Create the entry.  */
	exsys = tal(payz_top, struct payecs_external_system);
	exsys->system = tal_strdup(exsys, system);
	exsys->command = tal_strdup(exsys, command);
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
	ecs_register_func(&reg, &payecs_external_proxy);
	for (i = 0; i < tal_count(exsys->required); ++i)
		ecs_register_require(&reg, exsys->required[i]);
	for (i = 0; i < tal_count(exsys->disallowed); ++i)
		ecs_register_disallow(&reg, exsys->disallowed[i]);
	ecs_register_done(&reg);
	ecs_register(payz_top->ecs, take(reg));

	return true;
}

/*-----------------------------------------------------------------------------
External Proxy
-----------------------------------------------------------------------------*/

/*~
 * This proxy connects our ECS to the external plugins that have
 * registered to add their own systems to the payecs framework.
 */

struct payecs_external_proxy_closure {
	struct plugin *plugin;
	const char *system;
	u32 entity;
};

static struct command_result *
payecs_external_proxy_ok(struct command *command,
			 const char *buf,
			 const jsmntok_t *result,
			 struct payecs_external_proxy_closure *closure);
static struct command_result *
payecs_external_proxy_ng(struct command *command,
			 const char *buf,
			 const jsmntok_t *result,
			 struct payecs_external_proxy_closure *closure);

static struct command_result *
payecs_external_proxy(struct plugin *plugin,
		      struct ecs *ecs,
		      const char *system,
		      u32 entity)
{
	struct payecs_external_system *exsys;

	const char *compbuf;
	const jsmntok_t *comptok;

	struct out_req *req;
	struct json_stream *js;

	struct payecs_external_proxy_closure *closure;

	size_t i;

	/* Look it up.  */
	exsys = strmap_get(&payecs_registry, system);
	assert(exsys);

	/* Create the closure.  */
	closure = tal(plugin, struct payecs_external_proxy_closure);
	closure->plugin = plugin;
	closure->system = exsys->system;  /* Ensure storage is permanent.  */
	closure->entity = entity;

	/* Now generate the arguments to the external command.  */
	req = jsonrpc_request_start(plugin, NULL,
				    exsys->command,
				    &payecs_external_proxy_ok,
				    &payecs_external_proxy_ng,
				    closure);
	js = req->js;
	/* Pass the system name.
	 *
	 * Plugins might want to register multiple systems for
	 * code organization, but want to expose only one command
	 * for systems.
	 * This allows such plugins to branch in a single command
	 * based on the actual system being invoked.
	 */
	json_add_string(js, "system", system);

	json_object_start(js, "entity");
	json_add_u32(js, "entity", entity);
	/* As a convenience to the system, so it does not have to
	 * go through the overhead of JSONRPC processing to get
	 * the data it needs, also pass in the required components.
	 */
	for (i = 0; i < tal_count(exsys->required); ++i) {
		ecs_get_component(payz_top->ecs, &compbuf, &comptok,
				  entity, exsys->required[i]);
		json_add_tok(js, exsys->required[i],
			     comptok, compbuf);
	}
	json_object_end(js);

	return send_outreq(plugin, req);
}

static struct command_result *
payecs_external_proxy_ok(struct command *command,
			 const char *buf,
			 const jsmntok_t *result,
			 struct payecs_external_proxy_closure *closure)
{
	/* Success, do nothing.  */
	tal_steal(tmpctx, closure);
	return ecs_system_done(closure->plugin, payz_top->ecs);
}

static struct command_result *
payecs_external_proxy_ng(struct command *command,
			 const char *buf,
			 const jsmntok_t *result,
			 struct payecs_external_proxy_closure *closure)
{
	/* Command failed, update the `lightningd:systems` component
	 * and add the error.
	 */
	const char *compbuf;
	const jsmntok_t *comptok;

	size_t i;
	const jsmntok_t *key;
	const jsmntok_t *value;

	struct json_stream *js;

	const char *newcompbuf;
	size_t newcomplen;

	tal_steal(tmpctx, closure);

	/* Get the current lightningd:systems.  */
	ecs_get_component(payz_top->ecs, &compbuf, &comptok,
			  closure->entity, "lightningd:systems");

	/* Create the new lightningd:systems.  */
	js = new_json_stream(tmpctx, NULL, NULL);
	json_object_start(js, NULL);
	if (likely(comptok->type == JSMN_OBJECT)) {
		/* Copy the keys from the current object, except for
		 * an error field.  */
		json_for_each_obj (i, key, comptok) {
			value = key + 1;
			/* Skip an existing "error" field.  */
			if (json_tok_streq(compbuf, key, "error"))
				continue;

			json_add_tok(js, json_strdup(tmpctx, compbuf, key),
				     value, compbuf);
		}
	}
	/* Add the error.  */
	json_add_tok(js, "error", result, buf);
	json_object_end(js);

	/* Set the value.  */
	newcompbuf = json_out_contents(js->jout, &newcomplen);
	ecs_set_component_datuml(payz_top->ecs,
				 closure->entity, "lightningd:systems",
				 newcompbuf, newcomplen);

	return ecs_system_done(closure->plugin, payz_top->ecs);
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
	const char *command;
	const char **required;
	const char **disallowed;

	if (!param(cmd, buf, params,
		   p_req("system", &param_string, &system),
		   p_req("command", &param_string, &command),
		   p_req("required", &param_array_of_strings,
			 &required),
		   p_opt("disallowed", &param_array_of_strings,
			 &disallowed),
		   NULL))
		return command_param_failed();

	if (!payecs_register(system, take(command),
			     take(required), take(disallowed)))
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
	json_splice_default_systems(out);
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
	json_object_start(js, NULL);
	json_array_start(js, "systems");
	for (i = 0; i < tal_count(prepend); ++i)
		json_add_string(js, NULL, prepend[i]);
	json_splice_default_systems(js);
	for (i = 0; i < tal_count(append); ++i)
		json_add_string(js, NULL, append[i]);
	json_array_end(js);
	json_object_end(js);

	/* Set the value.  */
	newcompbuf = json_out_contents(js->jout, &newcomplen);
	ecs_set_component_datuml(payz_top->ecs,
				 (u32) *entity, "lightningd:systems",
				 newcompbuf, newcomplen);

	/* Return empty object.  */
	return command_success(cmd, json_out_obj(cmd, NULL, NULL));
}
