#include"ecs.h"
#include<assert.h>
#include<ccan/compiler/compiler.h>
#include<ccan/likely/likely.h>
#include<ccan/strmap/strmap.h>
#include<ccan/tal/str/str.h>
#include<common/status_levels.h>
#include<common/utils.h>
#include<plugins/libplugin.h>
#include<plugins/payz/ecs/ec.h>
#include<plugins/payz/ecs/ecsys.h>

/*-----------------------------------------------------------------------------
ECS Object Construction
-----------------------------------------------------------------------------*/


struct ecs_system_wrapper {
	char *name;
	ecs_system_function func;
	const char **required;
};

struct ecs {
	struct ec *ec;
	struct ecsys *ecsys;
	STRMAP(struct ecs_system_wrapper *) system_funcs;
};

static void wrapped_plugin_log(struct plugin *plugin,
			       enum log_level level,
			       const char *msg);
static bool wrapped_get_component(const void *ec,
				  const char **buffer,
				  const jsmntok_t **toks,
				  u32 entity,
				  const char *component);

static void ecs_destructor(struct ecs *ecs);

struct ecs *ecs_new(const tal_t *ctx)
{
	struct ecs *ecs = tal(ctx, struct ecs);

	ecs->ec = ec_new(ecs);
	ecs->ecsys = ecsys_new(ecs,
			       &wrapped_get_component,
			       &ec_set_component,
			       ecs->ec,
			       &plugin_notification_start,
			       &plugin_notification_end,
			       &wrapped_plugin_log);
	strmap_init(&ecs->system_funcs);
	tal_add_destructor(ecs, &ecs_destructor);

	return ecs;
}

static void wrapped_plugin_log(struct plugin *plugin,
			       enum log_level level,
			       const char *msg)
{
	plugin_log(plugin, level, "%s", msg);
}

static bool wrapped_get_component(const void *ec,
				  const char **buffer,
				  const jsmntok_t **toks,
				  u32 entity,
				  const char *component)
{
	return ec_get_component(ec, buffer, toks, entity, component);
}

static void ecs_destructor(struct ecs *ecs)
{
	/* Clean up strmap, it uses malloc.  */
	strmap_clear(&ecs->system_funcs);
}

/*-----------------------------------------------------------------------------
Delegation to EC
-----------------------------------------------------------------------------*/

u32 ecs_newentity(struct ecs *ecs)
{
	return ec_newentity(ecs->ec);
}

void ecs_get_entity_bounds(const struct ecs *ecs,
			   u32 *min,
			   u32 *max)
{
	return ec_get_entity_bounds(ecs->ec, min, max);
}

char **ecs_get_components(const tal_t *ctx,
			  const struct ecs *ecs,
			  u32 entity)
{
	return ec_get_components(ctx, ecs->ec, entity);
}

bool ecs_get_component(const struct ecs *ecs,
		       const char **buffer,
		       const jsmntok_t **toks,
		       u32 entity,
		       const char *component)
{
	return ec_get_component(ecs->ec, buffer, toks, entity, component);
}

void ecs_set_component(struct ecs *ecs,
		       u32 entity,
		       const char *component,
		       const char *buffer,
		       const jsmntok_t *tok)
{
	return ec_set_component(ecs->ec, entity, component, buffer, tok);
}

void ecs_set_component_datuml(struct ecs *ecs,
			      u32 entity,
			      const char *component,
			      const char *value,
			      size_t len)
{
	return ec_set_component_datuml(ecs->ec,
				       entity, component,
				       value, len);
}

void ecs_set_component_datum(struct ecs *ecs,
			     u32 entity,
			     const char *component,
			     const char *valuez)
{
	return ec_set_component_datum(ecs->ec, entity, component, valuez);
}

/*-----------------------------------------------------------------------------
Delegation to ECSYS
-----------------------------------------------------------------------------*/

struct ecs_advance_wrapper {
	struct ecs *ecs;
	struct command_result *(*cb)(struct plugin *,
				     struct ecs *,
				     void *cbarg);
	struct command_result *(*errcb)(struct plugin *,
					struct ecs *,
					errcode_t,
					void *cbarg);
	void *cbarg;
};
static struct command_result *
ecs_advance_wrapper_cb(struct plugin *plugin,
		       struct ecsys *ecsys UNUSED,
		       struct ecs_advance_wrapper *wrapper);
static struct command_result *
ecs_advance_wrapper_errcb(struct plugin *plugin,
			  struct ecsys *ecsys UNUSED,
			  errcode_t error,
			  struct ecs_advance_wrapper *wrapper);

struct command_result *ecs_advance_(struct plugin *plugin,
				    struct ecs *ecs,
				    u32 entity,
				    struct command_result *(*cb)(struct plugin *,
					    			 struct ecs *,
								 void *cbarg),
				    struct command_result *(*errcb)(struct plugin *,
								    struct ecs *,
								    errcode_t,
								    void *cbarg),
				    void *cbarg)
{
	struct ecs_advance_wrapper *wrapper;

	wrapper = tal(ecs, struct ecs_advance_wrapper);
	wrapper->ecs = ecs;
	wrapper->cb = cb;
	wrapper->errcb = errcb;
	wrapper->cbarg = cbarg;

	return ecsys_advance(plugin,
			     ecs->ecsys,
			     entity,
			     &ecs_advance_wrapper_cb,
			     &ecs_advance_wrapper_errcb,
			     wrapper);
	return NULL;
}

static struct command_result *
ecs_advance_wrapper_cb(struct plugin *plugin,
		       struct ecsys *ecsys UNUSED,
		       struct ecs_advance_wrapper *wrapper)
{
	tal_steal(tmpctx, wrapper);
	return wrapper->cb(plugin, wrapper->ecs, wrapper->cbarg);
}

static struct command_result *
ecs_advance_wrapper_errcb(struct plugin *plugin,
			  struct ecsys *ecsys UNUSED,
			  errcode_t error,
			  struct ecs_advance_wrapper *wrapper)
{
	tal_steal(tmpctx, wrapper);
	return wrapper->errcb(plugin, wrapper->ecs, error, wrapper->cbarg);
}

struct command_result *ecs_advance_done(struct command *command,
					struct ecs *ecs,
					u32 entity)
{
	ecsys_advance_done(command->plugin, ecs->ecsys, entity);
	return ecs_done(command, ecs);
}
struct command_result *ecs_done(struct command *command,
				struct ecs *ecs)
{
	return notification_handled(command);
}

bool ecs_system_exists(const struct ecs *ecs,
		       const char *system)
{
	return ecsys_system_exists(ecs->ecsys, system);
}

/*-----------------------------------------------------------------------------
Triggering of Built-in Systems
-----------------------------------------------------------------------------*/

struct command_result *ecs_system_notify(struct ecs *ecs,
					 struct command *command,
					 const char *buffer,
					 const jsmntok_t *params)
{
	const char *system;
	const jsmntok_t *entity;

	u32 eid;

	struct ecs_system_wrapper *wrapper;
	
	const char *error;

	size_t i;

	error = json_scan(tmpctx, buffer, params,
			  "{system:%}",
			  JSON_SCAN_TAL(tmpctx, json_strdup, &system));
	if (error) {
		plugin_log(command->plugin, LOG_UNUSUAL,
			   "Triggered '%s' without 'system' parameter: "
			   "%.*s",
			   ECS_SYSTEM_NOTIFICATION,
			   json_tok_full_len(params),
			   json_tok_full(buffer, params));
		return notification_handled(command);
	}

	wrapper = strmap_get(&ecs->system_funcs, system);
	if (!wrapper)
		/* This is *not* unusual; non-builtin systems that
		 * were provided by other plugins will not be in
		 * our strmap.
		 */
		return notification_handled(command);

	entity = json_get_member(buffer, params, "entity");
	if (!entity) {
		plugin_log(command->plugin, LOG_UNUSUAL,
			   "Triggered '%s' without 'entity' parameter: "
			   "%.*s",
			   ECS_SYSTEM_NOTIFICATION,
			   json_tok_full_len(params),
			   json_tok_full(buffer, params));
		return notification_handled(command);
	}

	error = json_scan(tmpctx, buffer, entity,
			  "{entity:%}",
			  JSON_SCAN(json_to_u32, &eid));
	if (error) {
		plugin_log(command->plugin, LOG_UNUSUAL,
			   "Triggered '%s' without 'entity' ID: "
			   "'entity': %.*s",
			   ECS_SYSTEM_NOTIFICATION,
			   json_tok_full_len(entity),
			   json_tok_full(buffer, entity));
		return notification_handled(command);
	}

	/* Check all the required components are in the parameters.  */
	for (i = 0; i < tal_count(wrapper->required); ++i) {
		if (!json_get_member(buffer, entity, wrapper->required[i])) {
			plugin_log(command->plugin, LOG_UNUSUAL,
				   "Triggered '%s' on system '%s' "
				   "without being provided "
				   "required component '%s': %.*s",
				   ECS_SYSTEM_NOTIFICATION,
				   wrapper->name,
				   wrapper->required[i],
				   json_tok_full_len(entity),
				   json_tok_full(buffer, entity));
			return notification_handled(command);
		}
	}

	return wrapper->func(ecs, command, eid, buffer, entity);
}

/*-----------------------------------------------------------------------------
Registration
-----------------------------------------------------------------------------*/

static struct ecs_register_desc over_and_out = ECS_REGISTER_OVER_AND_OUT();

struct ecs_register_desc *ecs_register_begin(const tal_t *ctx)
{
	struct ecs_register_desc *array;
	array = tal_arr(ctx, struct ecs_register_desc, 1);
	array[0] = over_and_out;
	return array;
}

void ecs_register_concat(struct ecs_register_desc **parray,
			 const struct ecs_register_desc *to_add)
{
	size_t size;

	/* If empty, do nothing.  */
	if (unlikely(to_add[0].type == ECS_REGISTER_TYPE_OVER_AND_OUT))
		return;

	/* Figure out where to start.  */
	size = tal_count(*parray);
	/* Overwrite the current ECS_REGISTER_TYPE_OVER_AND_OUT.  */
	(*parray)[size - 1] = *to_add;

	do {
		++to_add;
		/* Append.  */
		tal_arr_expand(parray, *to_add);
	} while (to_add->type != ECS_REGISTER_TYPE_OVER_AND_OUT);
}

void ecs_register(struct ecs *ecs,
		  const struct ecs_register_desc *to_register TAKES)
{
	/* ECS registration may happen before tmpctx is set up.  */
	const char *owner = tal(ecs, char);

	const char *name = NULL;
	ecs_system_function func = NULL;
	const char **required = NULL;
	const char **disallowed = NULL;

	struct ecs_system_wrapper *wrapper;

	const struct ecs_register_desc *desc;

	if (taken(to_register))
		tal_steal(owner, to_register);

	for (desc = to_register;
	     desc->type != ECS_REGISTER_TYPE_OVER_AND_OUT;
	     ++desc) {
		switch (desc->type) {
		case ECS_REGISTER_TYPE_NAME:
			assert(!name);
			name = (const char*) desc->pointer;
			break; 

		case ECS_REGISTER_TYPE_FUNC:
			assert(!func);
			func = (ecs_system_function) desc->pointer;
			break;

		case ECS_REGISTER_TYPE_REQUIRE:
			assert(name);
			if (!required)
				required = tal_arr(owner, const char *, 0);
			tal_arr_expand(&required,
				       (const char*) desc->pointer);
			break;

		case ECS_REGISTER_TYPE_DISALLOW:
			assert(name && required);
			if (!disallowed)
				disallowed = tal_arr(owner, const char *, 0);
			tal_arr_expand(&disallowed,
				       (const char*) desc->pointer);
			break;

		case ECS_REGISTER_TYPE_DONE:
			assert(name);

			ecsys_register(ecs->ecsys, name,
				       required, tal_count(required),
				       disallowed, tal_count(disallowed));
			/* Also register to our layer if a function is
			 * declared.
			 */
			if (func) {
				wrapper = tal(ecs, struct ecs_system_wrapper);
				wrapper->name = tal_strdup(wrapper, name);
				wrapper->func = func;
				wrapper->required = tal_steal(wrapper,
							      required);
				strmap_add(&ecs->system_funcs,
					   wrapper->name, wrapper);

				/* wrapper took responsibility for it.  */
				required = NULL;
			}

			/* Clear the variables.  */
			name = NULL;
			func = NULL;
			required = tal_free(required);
			disallowed = tal_free(disallowed);
			break;

		case ECS_REGISTER_TYPE_OVER_AND_OUT:
			/* Impossible.  */
			abort();
		}
	}

	assert(!name);
	assert(!func);
	assert(!required);
	assert(!disallowed);

	tal_free(owner);
}

/*-----------------------------------------------------------------------------
Dynamic registration
-----------------------------------------------------------------------------*/

static
void ecs_register_extend(struct ecs_register_desc **parray,
			 enum ecs_register_type type,
			 const void *pointer)
{
	size_t n = tal_count(*parray);
	(*parray)[n - 1].type = type;
	(*parray)[n - 1].pointer = pointer;
	tal_arr_expand(parray, over_and_out);
}

void ecs_register_name(struct ecs_register_desc **parray,
		       const char *name TAKES)
{
	ecs_register_extend(parray, ECS_REGISTER_TYPE_NAME,
			    (const void *) tal_strdup(*parray, name));
}

void ecs_register_func(struct ecs_register_desc **parray,
		       ecs_system_function func)
{
	ecs_register_extend(parray, ECS_REGISTER_TYPE_FUNC,
			    (const void *) func);
}

void ecs_register_require(struct ecs_register_desc **parray,
			  const char *component TAKES)
{
	ecs_register_extend(parray, ECS_REGISTER_TYPE_REQUIRE,
			    (const void *) tal_strdup(*parray, component));
}

void ecs_register_disallow(struct ecs_register_desc **parray,
			   const char *component TAKES)
{
	ecs_register_extend(parray, ECS_REGISTER_TYPE_DISALLOW,
			    (const void *) tal_strdup(*parray, component));
}

void ecs_register_done(struct ecs_register_desc **parray)
{
	ecs_register_extend(parray, ECS_REGISTER_TYPE_DONE, NULL);
}
