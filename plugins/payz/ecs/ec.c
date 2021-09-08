#include"ec.h"
#include<assert.h>
#include<ccan/intmap/intmap.h>
#include<ccan/strmap/strmap.h>
#include<ccan/tal/str/str.h>
#include<common/json.h>
#include<common/utils.h>
#include<string.h>

/*~
 * The EC object is a mapping of entities to
 * struct ec_entityrow.
 * A struct ec_entityrow is a mapping of
 * component names to struct ec_cell.
 * An ec_cell is really just a string buffer
 * and pre-parsed JSON token.
 */

/** struct ec_cell
 *
 * @brief Represents a JSON datum of a component,
 */
struct ec_cell {
	char *buffer;
	jsmntok_t *tok;
};

/** struct ec_entityrow
 *
 * @brief Represents the components attached to an
 * entity.
 */
struct ec_entityrow {
	/** Mapping from component name to cell.  */
	STRMAP(struct ec_cell *) component_map;
};

struct ec {
	/** The next entity available to allocate.  */
	u32 next_entity;

	/** Tal-allocated "null" JSON datum, used to return an
	 * ec_getcomponent call if the requested component
	 * is not attached.
	 */
	char *null_buffer;
	jsmntok_t *null_tok;

	/** Mapping from entity ID to entity row.  */
	UINTMAP(struct ec_entityrow *) entity_map;
};

static void destroy_ecs(struct ec *ec);
static void destroy_entityrow(struct ec_entityrow *entityrow);

struct ec *ec_new(const tal_t *ctx)
{
	struct ec *ec = tal(ctx, struct ec);

	ec->next_entity = 1;
	ec->null_buffer = tal_strdup(ec, "null");
	ec->null_tok = tal_arr(ec, jsmntok_t, 1);
	ec->null_tok[0].type = JSMN_PRIMITIVE;
	ec->null_tok[0].start = 0;
	ec->null_tok[0].end = 4;
	ec->null_tok[0].size = 0;
	uintmap_init(&ec->entity_map);

	tal_add_destructor(ec, &destroy_ecs);

	return ec;
}

u32 ec_newentity(struct ec *ec)
{
	return ec->next_entity++;
}

void ec_get_entity_bounds(const struct ec *ec,
			   u32 *min,
			   u32 *max)
{
	intmap_index_t im_min;
	intmap_index_t im_max;
	if (uintmap_empty(&ec->entity_map)) {
		*min = 0;
		*max = 0;
		return;
	}

	(void) uintmap_first(&ec->entity_map, &im_min);
	(void) uintmap_last(&ec->entity_map, &im_max);

	*min = (u32) im_min;
	*max = (u32) im_max + 1;
}

static bool ec_get_components_from_strmap(const char *name,
					   struct ec_cell *value,
					   char ***components);
char **ec_get_components(const tal_t *ctx,
			  const struct ec *ec,
			  u32 entity)
{
	char **components = NULL;
	struct ec_entityrow *entityrow = uintmap_get(&ec->entity_map,
						      entity);

	if (!entityrow)
		return NULL;

	components = tal_arr(ctx, char*, 0);
	strmap_iterate(&entityrow->component_map,
		       &ec_get_components_from_strmap, &components);
	/* If the components were empty then the entityrow should have
	 * been deleted.  */
	assert(tal_count(components) != 0);

	return components;
}
static bool ec_get_components_from_strmap(const char *name,
					   struct ec_cell *value,
					   char ***components)
{
	tal_arr_expand(components, tal_strdup(*components, name));
	return true;
}

bool ec_get_component(const struct ec *ec,
		       const char **buffer,
		       const jsmntok_t **toks,
		       u32 entity,
		       const char *component)
{
	struct ec_entityrow *entityrow;
	struct ec_cell *cell;

	entityrow = uintmap_get(&ec->entity_map, entity);
	if (!entityrow) {
		*buffer = ec->null_buffer;
		*toks = ec->null_tok;
		return false;
	}

	cell = strmap_get(&entityrow->component_map, component);
	if (!cell) {
		*buffer = ec->null_buffer;
		*toks = ec->null_tok;
		return false;
	}

	*buffer = cell->buffer;
	*toks = cell->tok;
	return true;
}

static struct ec_entityrow *ec_entityrow_new(const tal_t *ctx);
static struct ec_cell *ec_cell_new(const tal_t *ctx,
				   const char *buffer,
				   const jsmntok_t *tok);
void ec_set_component(struct ec *ec,
		       u32 entity,
		       const char *component,
		       const char *buffer,
		       const jsmntok_t *tok)
{
	bool detach = false;
	struct ec_entityrow *entityrow;
	struct ec_cell *cell;

	if (!buffer || !tok) {
		assert(!buffer && !tok);
		detach = true;
	}

	if (json_tok_is_null(buffer, tok))
		detach = true;

	entityrow = uintmap_get(&ec->entity_map, entity);

	if (detach) {
		/* Nothing to detach?  */
		if (!entityrow)
			return;
		cell = strmap_get(&entityrow->component_map, component);
		if (!cell)
			return;

		strmap_del(&entityrow->component_map, component, NULL);
		tal_free(cell);

		/* If entity row is now empty, also delete the row.  */
		if (strmap_empty(&entityrow->component_map)) {
			uintmap_del(&ec->entity_map, entity);
			tal_free(entityrow);
		}
	} else {
		/* No components yet?  */
		if (!entityrow) {
			entityrow = ec_entityrow_new(ec);
			uintmap_add(&ec->entity_map, entity, entityrow);
			cell = NULL;
		} else
			cell = strmap_get(&entityrow->component_map, component);

		/* Detach first if already exist.  */
		if (cell) {
			strmap_del(&entityrow->component_map, component, NULL);
			cell = tal_free(cell);
		}

		/* Now attach.  */
		cell = ec_cell_new(entityrow, buffer, tok);
		strmap_add(&entityrow->component_map, component, cell);
	}
}

static struct ec_entityrow *ec_entityrow_new(const tal_t *ctx)
{
	struct ec_entityrow *entityrow = tal(ctx, struct ec_entityrow);

	strmap_init(&entityrow->component_map);

	tal_add_destructor(entityrow, &destroy_entityrow);

	return entityrow;
}

static struct ec_cell *ec_cell_new(const tal_t *ctx,
				   const char *buffer,
				   const jsmntok_t *tok)
{
	struct ec_cell *cell = tal(ctx, struct ec_cell);

	/* Determine text buffer size to copy.  */
	const char *to_copy = json_tok_full(buffer, tok);
	int len = json_tok_full_len(tok);
	/* Determine token array size to copy.  */
	const jsmntok_t *tok_end = json_next(tok);

	/* The offset to apply to all copied tokens.  */
	int offset = to_copy - buffer;

	/* Load the cell.  */
	cell->buffer = tal_dup_arr(cell, char,
				   to_copy, len, 0);
	cell->tok = tal_dup_arr(cell, jsmntok_t,
				tok, tok_end - tok, 0);
	/* Adjust the copied tokens by the offset.  */
	if (offset != 0) {
		int i;
		for (i = 0; i < tal_count(cell->tok); ++i) {
			cell->tok[i].start -= offset;
			cell->tok[i].end -= offset;
		}
	}

	return cell;
}

void ec_set_component_datuml(struct ec *ec,
			     u32 entity,
			     const char *component,
			     const char *value,
			     size_t len)
{
	ec_set_component(ec, entity, component, value,
			 json_parse_simple(tmpctx, value, len));
}

void ec_set_component_datum(struct ec *ec,
			    u32 entity,
			    const char *component,
			    const char *valuez)
{
	ec_set_component_datuml(ec, entity, component, valuez,
				strlen(valuez));
}

/*-----------------------------------------------------------------------------
EC Destructor
-----------------------------------------------------------------------------*/
/*~
 * The ccan intmap and strmap modules do not use tal, but instead
 * use malloc.
 *
 * To ensure that malloc-allocated objects get freed when the EC
 * object itself is freed, we need to clear each entity row
 * strmap, then clear the entity intmap.
 */

static void destroy_entityrow(struct ec_entityrow *entityrow)
{
	/* Cells are tal-allocated, so no need to delete the
	 * contained cells.  */

	strmap_clear(&entityrow->component_map);
}

static void destroy_ecs(struct ec *ec)
{
	/* Entity rows are tal-allocated, so no need to delete
	 * the contained rows.
	 */

	uintmap_clear(&ec->entity_map);
}
