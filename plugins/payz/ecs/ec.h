#ifndef LIGHTNING_PLUGINS_PAYZ_ECS_EC_H
#define LIGHTNING_PLUGINS_PAYZ_ECS_EC_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<external/jsmn/jsmn.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdlib.h>

/** struct ec
 *
 * @brief Represents an instance of an Entity-Component table,
 * which attaches named components to numeric ID entities.
 *
 * @desc This implements the data of an ECS framework object.
 */
struct ec;

/** ec_new
 *
 * @brief Constructs an instance of the Entity-Component table.
 *
 * @param ctx - The owner of this EC table.
 */
struct ec *ec_new(const tal_t *ctx);

/** ec_newentity
 *
 * @brief Allocate a fresh entity ID number.
 *
 * @param ec - The EC instance to allocaate from.
 *
 * @return - An entity ID that no previous ec_newentity on
 * this instance has returned.
 * Never 0.
 */
u32 ec_newentity(struct ec *ec);

/** ec_get_entity_bounds
 *
 * @brief Return the minimum and maximum bounds of entities
 * in this EC instance.
 * The minimum bound is inclusive, the maximum bound is
 * exclusive, i.e. min <= entity < max.
 *
 * @desc This provides bounds that may be looser than actual.
 * It will return min == max if there are no entities with
 * any attached components.
 *
 * @param ec - the EC instance to query.
 * @param min - output, the minimum entity.
 * @param max - output, one past the maximum entity.
 */
void ec_get_entity_bounds(const struct ec *ec,
			   u32 *min,
			   u32 *max);

/** ec_get_components
 *
 * @brief Gets the component names of components attached to
 * the given entity.
 *
 * @param ctx - the tal context to allocate the returned
 * array from.
 * @param ec - the EC instance to query.
 * @param entity - the entity whose attached components will
 * be returned.
 *
 * @return - a tal-allocated array of null-terminated strings
 * naming the components attached to this entity.
 * The array has the given ctx as parent, while each string
 * has the array as parent.
 * Return NULL if the entity has no attached components.
 * The returned array is sorted by a trivial byte-based
 * compare (i.e. strcmp).
 */
char **ec_get_components(const tal_t *ctx,
			  const struct ec *ec,
			  u32 entity);

/** ec_get_component
 *
 * @brief Gets the value of the given component attached to the
 * given entity.
 *
 * @param ec - the EC instance to query.
 * @param buffer - output, the tal-allocated string buffer
 * containing the raw JSON text.
 * The ec instance owns the storage for this string buffer,
 * and the storage may be invalidated if you run
 * ec_set_component afterwards.
 * The buffer is *not* null-terminated; use the toks->end
 * below to determine the usable extent of the buffer.
 * @param toks - output, the tal-allocated array of tokens
 * representing the JSON value.
 * The ec instance owns the storage for this token array,
 * and the storage may be invalidated if you run
 * ec_set_component afterwards.
 * @param entity - the numeric ID of the entity to look up.
 * @param component - the name of the component to look up.
 *
 * @return - true if the entity has the component attached,
 * false if the entity does not have the component attached.
 * Note that if this returns false, buffer and toks are still
 * set to a "null" JSON datum.
 */
bool ec_get_component(const struct ec *ec,
		       const char **buffer,
		       const jsmntok_t **toks,
		       u32 entity,
		       const char *component);

/** ec_set_component
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
 * @param ec - The EC instance to mutate.
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
void ec_set_component(struct ec *ec,
		       u32 entity,
		       const char *component,
		       const char *buffer,
		       const jsmntok_t *tok);

/** ec_set_component_datuml
 *
 * @brief Like ec_set_component, but accepts a buffer and
 * length containing valid JSON text.
 */
void ec_set_component_datuml(struct ec *ec,
			     u32 entity,
			     const char *component,
			     const char *value,
			     size_t len);

/** ec_set_component_datum
 *
 * @brief Like ec_set_component, but accepts a null-terminated
 * string containing valid JSON text.
 */
void ec_set_component_datum(struct ec *ec,
			    u32 entity,
			    const char *component,
			    const char *valuez);

/** ec_detach
 *
 * @brief A convenient shortcut macro to use ec_set_component
 * to detach a component.
 */
#define ec_detach(ec, entity, component) \
	ec_set_component((ec), (entity), (component), NULL, NULL)

#endif /* LIGHTNING_PLUGINS_PAYZ_EC_H */
