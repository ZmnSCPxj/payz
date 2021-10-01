#ifndef LIGHTNING_PLUGINS_PAYZ_ECS_ECS_H
#define LIGHTNING_PLUGINS_PAYZ_ECS_ECS_H
#include"config.h"
#include<ccan/take/take.h>
#include<ccan/take/take.h>
#include<ccan/tal/tal.h>
#include<ccan/typesafe_cb/typesafe_cb.h>
#include<common/json.h>
#include<plugins/payz/ecs/ecsys.h>
#include<stdbool.h>
#include<stddef.h>

struct command;
struct command_result;
struct plugin;

/** struct ecs
 *
 * @brief Represents a full Entity Component System
 * framework, allowing entities to have attached
 * components, and for systems to operate on particular
 * entities.
 */
struct ecs;

/** ecs_new
 *
 * @brief Constructs a new ECS framework.
 *
 * @param ctx - the owner of this ECS framework.
 */
struct ecs *ecs_new(const tal_t *ctx);

/** ecs_newentity
 *
 * @brief Allocate a fresh entity ID number.
 *
 * @param ecs - the ECS framework to allocate from.
 *
 * @return - A non-zero entity ID that no previous
 * ecs_newentity on this instance has returned.
 */
u32 ecs_newentity(struct ecs *ecs);

/** ecs_get_entity_bounds
 *
 * @brief Return the minimum and maximum bounds of entities
 * with at least one component, in this ECS framework
 * instance.
 * min <= entity < max.
 * Returns min == max if no entities have components.
 *
 * @param ecs - the ECS framework to query.
 * @param min - output, the minimum entity.
 * @param max - output, one past the maximum entity.
 */
void ecs_get_entity_bounds(const struct ecs *ecs,
			   u32 *min,
			   u32 *max);

/** ecs_get_components
 *
 * @brief Gets the component names of components attached to
 * the given entity.
 *
 * @param ctx - the tal context to allocate the returned
 * array from.
 * @param ecs - the ECS framework to query.
 * @param entity - the entity whose attached components will
 * be returned.
 *
 * @return - a tal-allocated array of null-terminated strings
 * naming the components attached to this entity.
 * The array has the given ctx as parent, while each string
 * has the array as parent.
 * Return NULL if the entity has no attached components.
 */
char **ecs_get_components(const tal_t *ctx,
			  const struct ecs *ecs,
			  u32 entity);

/** ecs_get_component
 *
 * @brief Gets the value of the given component attached to
 * the given entity.
 *
 * @param ecs - the ECS framework to query.
 * @param buffer - output, the tal-allocated string buffer
 * containing the raw JSON text.
 * The ECS frameowrk owns the storage for this string buffer,
 * and the storage may be invalidated if you run
 * ecs_set_component afterwards.
 * The buffer is *not* null-terminated; use the toks->end
 * below to determine the usable extent of the buffer.
 * @param toks - output, the tal-allocated array of tokens
 * representing the JSON value.
 * The ECS framework owns the storage for this token array,
 * and the storage may be invalidated if you run
 * ecs_set_component afterwards.
 * @param entity - the numeric ID of the entity to look up.
 * @param component - the name of the component to look up.
 *
 * @return - true if the entity has the component attached,
 * false if the entity does not have the component attached.
 * Note that if this returns false, buffer and toks are still
 * set to a "null" JSON datum.
 */
bool ecs_get_component(const struct ecs *ecs,
		       const char **buffer,
		       const jsmntok_t **toks,
		       u32 entity,
		       const char *component);

/** ecs_set_component
 *
 * @brief Attaches, detaches, or mutates the component
 * attached to the given entity.
 *
 * @desc If the given value is a JSON null or a C NULL,
 * then the component is detached if it is currently
 * attached (no change if not currently attached).
 * Otherwise, it attaches the component or mutates an
 * existing component.
 *
 * If attaching or mutating, this creates a copy of the
 * given JSON object, owned by the given EC instance.
 *
 * @param ecs - The ECS framework to mutate.
 * @param entity - the numeric ID of the entity to mutate.
 * @param component - the name of the component to mutate.
 * @param buffer - the string buffer containing raw JSON
 * text.
 * May be NULL (in which case tok must be also NULL) to
 * detach.
 * @param tok - the JSON top object to set as the component
 * of the given entity.
 * May be NULL (in which case buffer must also be NULL),
 * or point to a JSON null object, to detach.
 */
void ecs_set_component(struct ecs *ecs,
		       u32 entity,
		       const char *component,
		       const char *buffer,
		       const jsmntok_t *tok);

/** ecs_set_component_datuml
 *
 * @brief Like ecs_set_component, but accepts a buffer and
 * length containing valid JSON text.
 */
void ecs_set_component_datuml(struct ecs *ecs,
			      u32 entity,
			      const char *component,
			      const char *value,
			      size_t len);

/** ecs_set_component_datum
 *
 * @brief Like ecs_set_component, but accepts a null-terminated
 * string containing valid JSON text.
 */
void ecs_set_component_datum(struct ecs *ecs,
			     u32 entity,
			     const char *component,
			     const char *valuez);

/** ecs_advance
 *
 * @brief Advances processing of the specified entity, triggering
 * the next system in its `lightningd:systems` component.
 *
 * @param plugin - The plugin this is running in.
 * @param ecs - The ECS framework.
 * @param entity - The entity ID to advance.
 * @param cb - the function to call in case of success.
 * @param errcb - the function to call in case of failure.
 * @param cbarg - the object to pass to the above functions.
 */
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
				    void *cbarg);
#define ecs_advance(plugin_o, ecs_o, entity, cb, errcb, cbarg) \
	ecs_advance_((plugin_o), (ecs_o), (entity), \
		     typesafe_cb_preargs(struct command_result *, void *, \
					 (cb), (cbarg), \
					 struct plugin *, \
					 struct ecs *), \
		     typesafe_cb_preargs(struct command_result *, void *, \
					 (errcb), (cbarg), \
					 struct plugin *, \
					 struct ecs *, \
					 errcode_t), \
		     (cbarg))

/** ecs_advance_done
 *
 * @brief Advances and signals that the system has completed
 * execution.
 * Must be called from system code.
 *
 * @desc Our expectation is that the supermajority of systems
 * will, at the end of their processing, simply terminate.
 */
void ecs_advance_done(struct plugin *plugin,
		      struct ecs *ecs,
		      u32 entity);

/** ecs_system_exists
 *
 * @brief Check if a system of a specific name is already
 * registered.
 *
 * @param ecs - the ECS framework to query.
 * @param system - the name of the system to check.
 *
 * @return - true if a system of the given name is already
 * registered, false otherwise.
 */
bool ecs_system_exists(const struct ecs *ecs,
		       const char *system);

/** ecs_system_notify
 *
 * @brief Call the actual system code if a function was
 * provided during registration.
 * Otherwise if the system has no function provided
 * during registration, do nothing.
 *
 * @desc This is intended to be called from the handler
 * of the payecs_system_trigger notification.
 *
 * @param ecs - the ECS framework to trigger.
 * @param command - the command argument passed into the
 * plugin notification handler.
 * @param buffer - the buffer for JSON parameters.
 * @param params - the JSMN token for parameters of the
 * system triggering notification.
 */
void ecs_system_notify(struct ecs *ecs,
		       struct command *command,
		       const char *buffer,
		       const jsmntok_t *params);

/*-----------------------------------------------------------------------------
System Registration
-----------------------------------------------------------------------------*/

/*
To use:

void system_name_code(struct ecs *,
		      struct command *,
		      const char *buffer,
		      const jsmntok_t *entity);
void other_system_name_code(struct ecs *,
			    struct command *,
			    const char *buffer,
			    const jsmntok_t *entity);

const struct ecs_register_desc desc[] = {
	ECS_REGISTER_NAME("system-name"),
	ECS_REGISTER_FUNC(&system_name_code),
	ECS_REGISTER_REQUIRE("component-name"),
	ECS_REGISTER_REQUIRE("component-name-2"),
	ECS_REGISTER_DISALLOW("bad-component-name"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("other-system-name"),
	ECS_REGISTER_FUNC(&other_system_name_code),
	ECS_REGISTER_REQUIRE("some-component-name"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_OVER_AND_OUT()
};

Then:

	ecs_register(ecs, desc);
*/

enum ecs_register_type {
	ECS_REGISTER_TYPE_OVER_AND_OUT = 0,
	ECS_REGISTER_TYPE_NAME,
	ECS_REGISTER_TYPE_FUNC,
	ECS_REGISTER_TYPE_REQUIRE,
	ECS_REGISTER_TYPE_DISALLOW,
	ECS_REGISTER_TYPE_DONE
};

struct ecs_register_desc {
	enum ecs_register_type type;
	const void *pointer;
};

/* The entity given is an object, with a field "entity" with a numeric
 * entity id, and the rest of the fields being components that were
 * registered as required.
 */
typedef
void (*ecs_system_function)(struct ecs *, struct command *,
			    const char *buffer, const jsmntok_t *entity);

#define ECS_REGISTER_NAME(name) \
	{ ECS_REGISTER_TYPE_NAME, \
	  typesafe_cb_cast(const void *, const char *, (name)) }
/* Kinda assumes that sizeof(void*) == sizeof(void (*)(void)), which is
 * not true on some really weird hardware (16-bit x86), but those hardware
 * are unlikely to run anything Unix-like anyway and we are dependent on
 * sockets and pipes and processes and other Unixy things.
 */
#define ECS_REGISTER_FUNC(func) \
	{ ECS_REGISTER_TYPE_FUNC, \
	  typesafe_cb_cast(const void *, \
			   ecs_system_function, \
			   (func)) }
#define ECS_REGISTER_REQUIRE(component) \
	{ ECS_REGISTER_TYPE_REQUIRE, \
	  typesafe_cb_cast(const void *, const char *, (component)) }
#define ECS_REGISTER_DISALLOW(component) \
	{ ECS_REGISTER_TYPE_DISALLOW, \
	  typesafe_cb_cast(const void *, const char *, (component)) }
#define ECS_REGISTER_DONE() \
	{ ECS_REGISTER_TYPE_DISALLOW, NULL }
#define ECS_REGISTER_OVER_AND_OUT() \
	{ ECS_REGISTER_TYPE_OVER_AND_OUT, NULL }

/** ecs_register_begin
 *
 * @brief Constructs an array that would allow ecs_register_concat
 * to concatenate a static descriptor later.
 *
 * @param ctx - The owner of this array.
 *
 * @return - An array appropriate for registration, containing only
 * ECS_REGISTER_OVER_AND_OUT().
 */
struct ecs_register_desc *ecs_register_begin(const tal_t *ctx);

/** ecs_register_concat
 *
 * @brief in combination with the above, used to concatenate
 * multiple separate static arrays into a single dynamic array
 * for passing to ecs_register.
 *
 * @param parray - pointer to the variable containing the array,
 * from ecs_register_begin.
 * @param to_add - the array being added, must be terminated by
 * ECS_REGISTER_OVER_AND_OUT().
 */
void ecs_register_concat(struct ecs_register_desc **parray,
			 const struct ecs_register_desc *to_add);

/** ecs_register
 * 
 * @brief Actually registers an array of ecs_register_desc entries.
 *
 * @param ecs - the ECS framework to register the system into.
 * @param to_register - the array to be processed.
 */
void ecs_register(struct ecs *ecs,
		  const struct ecs_register_desc *to_register TAKES);

/*~
 * The below are used in conjunction with ecs_register_begin to
 * perform "dynamic" registration.
 * They are congruent macro versions except they must be used in
 * code, in combination with ecs_register_begin, and not in
 * declarations.
 *
 * Notice there is no over-and-out --- the functions below
 * automatically append it.
 */
void ecs_register_name(struct ecs_register_desc **parray,
		       const char *name TAKES);
void ecs_register_func(struct ecs_register_desc **parray,
		       ecs_system_function func);
void ecs_register_require(struct ecs_register_desc **parray,
			  const char *component TAKES);
void ecs_register_disallow(struct ecs_register_desc **parray,
			   const char *component TAKES);
void ecs_register_done(struct ecs_register_desc **parray);

#define ECS_SYSTEM_NOTIFICATION ECSYS_SYSTEM_NOTIFICATION

#endif /* LIGHTNING_PLUGINS_PAYZ_ECS_ECS_H */
