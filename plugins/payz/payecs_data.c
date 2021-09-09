#include"payecs_data.h"
#include<ccan/array_size/array_size.h>
#include<ccan/strmap/strmap.h>
#include<ccan/json_escape/json_escape.h>
#include<common/json_stream.h>
#include<common/json_tok.h>
#include<common/param.h>
#include<plugins/payz/ecs/ecs.h>
#include<plugins/payz/top.h>
#include<string.h>

static struct command_result *
payecs_listentities(struct command *cmd,
		    const char *buf,
		    const jsmntok_t *params);
static struct command_result *
payecs_newentity(struct command *cmd,
		 const char *buf,
		 const jsmntok_t *params);
static struct command_result *
payecs_getcomponents(struct command *cmd,
		     const char *buf,
		     const jsmntok_t *params);
static struct command_result *
payecs_setcomponents(struct command *cmd,
		     const char *buf,
		     const jsmntok_t *params);

const struct plugin_command payecs_data_commands[] = {
	{
		"payecs_listentities",
		"payment",
		"List all entities with attached components, or those "
		"with {required} components and without {disallowed} "
		"components",
		"List entities.",
		&payecs_listentities
	},
	{
		"payecs_newentity",
		"payment",
		"Allocate an entity ID number.",
		"Allocate entity.",
		&payecs_newentity
	},
	{
		"payecs_getcomponents",
		"payment",
		"Look up {entity} for one or more {components}.",
		"Look up components of an entity.",
		&payecs_getcomponents
	},
	{
		"payecs_setcomponents",
		"payment",
		"Perform specified {writes}, after atomically ensuring that "
		"the optional {expected} still holds.",
		"Set components of an entity.",
		&payecs_setcomponents
	}
};
const size_t num_payecs_data_commands = ARRAY_SIZE(payecs_data_commands);

/*-----------------------------------------------------------------------------
Parameter Parsing: Array of Strings
-----------------------------------------------------------------------------*/

static struct command_result *
param_array_of_strings(struct command *cmd, const char *name,
		       const char *buffer, const jsmntok_t *tok,
		       const char ***arr)
{
	struct json_escape *esc;
	struct command_result *result;
	const jsmntok_t *j_arr;
	const jsmntok_t *j_str;
	size_t i;

	/* As a convenience, if a single string, promote to a 1-entry
	 * array of strings.  */
	if (tok->type == JSMN_STRING) {
		*arr = tal_arr(cmd, const char *, 1);
		esc = json_escape_string_(cmd, buffer + tok->start,
					  tok->end - tok->start);
		(*arr)[0] = json_escape_unescape(cmd, esc);
		if (!(*arr)[0])
			goto fail;
		return NULL;
	}

	result = param_array(cmd, name, buffer, tok, &j_arr);
	if (result)
		return result;

	*arr = tal_arr(cmd, const char *, j_arr->size);
	json_for_each_arr (i, j_str, j_arr) {
		esc = json_escape_string_(cmd, buffer + j_str->start,
					  j_str->end - j_str->start);
		(*arr)[i] = json_escape_unescape(cmd, esc);
		if (!(*arr)[i])
			goto fail;
	}
	return NULL;

fail:
	return command_fail_badparam(cmd, name, buffer, tok,
				     "should be an array of strings.");
}

/*-----------------------------------------------------------------------------
Parameter Parsing: Write Specifications
-----------------------------------------------------------------------------*/

/** struct payecs_writespec
 *
 * @brief Represents a "write specification" as accepted as an argument by
 * `payecs_setcomponents` command, in its `writes` and `expected` parameters.
 */
struct payecs_writespec {
	/* The entity to write/validate.  */
	u32 entity;
	/* Whether the user wants an exact match (or to delete all existing
	 * components) or not.  */
	bool exact;
	/* The components to write.
	 * Since the raw JSON data are kept in the buffer passed in to
	 * commands, we only store the jsmntok_t here.
	 */
	STRMAP(const jsmntok_t *) components;
	/* Number of components above.  */
	size_t num_components;
};

/** json_to_payecs_writespec
 *
 * @brief Extract a payecs_writespec.
 */
static bool
json_to_payecs_writespec(const tal_t *ctx,
			 const char *buffer, const jsmntok_t *tok,
			 struct payecs_writespec *spec)
{
	bool entity_found = false;
	const jsmntok_t *key;
	const jsmntok_t *value;
	size_t i;

	if (tok->type != JSMN_OBJECT)
		return false;

	spec->exact = false;
	spec->num_components = 0;

	json_for_each_obj (i, key, tok) {
		value = key + 1;

		if (json_tok_streq(buffer, key, "entity")) {
			entity_found = true;
			if (!json_to_u32(buffer, value, &spec->entity))
				return false;
			continue;
		}
		if (json_tok_streq(buffer, key, "exact")) {
			if (!json_to_bool(buffer, value, &spec->exact))
				return false;
			continue;
		}

		/* We cannot output object keys with problematic
		 * characters (ccan/json_out/json_out.c, check_fieldname
		 * function), so prevent those from being written
		 * in the first place.
		 */
		if (json_escape_needed(buffer + key->start,
				       key->end - key->start))
			return false;

		++spec->num_components;
		strmap_add(&spec->components,
			   json_strdup(ctx, buffer, key),
			   value);
	}

	if (!entity_found)
		return false;

	return true;
}

/* Needed since strmap uses malloc, not tal.
 * We have to install this as a destructor since we cannot be
 * sure whether partial success (e.g. parse success in one
 * argument, but failure in another argument) will happen,
 * so work off destruction of struct command.
 */
static void
destroy_payecs_writespec_array(struct payecs_writespec *spec);

/** param_array_of_payecs_writespec
 *
 * @brief Parses an array of `payecs_setcomponents` `writes` and `expected`
 * arguments.
 * As a convenience, we also consider a single object.
 */
static struct command_result *
param_array_of_payecs_writespec(struct command *cmd,
				const char *name,
				const char *buffer,
				const jsmntok_t *tok,
				struct payecs_writespec **spec)
{
	const jsmntok_t *entry;
	size_t i;

	if (tok->type == JSMN_OBJECT) {
		/* Put it in a single-entry array.  */
		*spec = tal_arr(cmd, struct payecs_writespec, 1);
		strmap_init(&(*spec)[0].components);
		tal_add_destructor(*spec, &destroy_payecs_writespec_array);

		if (!json_to_payecs_writespec(cmd,
					      buffer, tok,
					      &(*spec)[0]))
			goto fail;

		return NULL;
	}

	if (tok->type != JSMN_ARRAY)
		goto fail;

	*spec = tal_arr(cmd, struct payecs_writespec, tok->size);
	/* Initialize the strmaps here and arrange to destroy
	 * later.  */
	for (i = 0; i < tal_count(*spec); ++i)
		strmap_init(&(*spec)[i].components);
	tal_add_destructor(*spec, &destroy_payecs_writespec_array);

	json_for_each_arr (i, entry, tok) {
		if (!json_to_payecs_writespec(cmd,
					      buffer, entry,
					      &(*spec)[i]))
			goto fail;
	}

	return NULL;

fail:
	return command_fail_badparam(cmd, name, buffer, tok,
				     "should be an array of entity-component "
				     "specifications.");
}

static void
destroy_payecs_writespec_array(struct payecs_writespec *spec)
{
	size_t i;
	for (i = 0; i < tal_count(spec); ++i)
		strmap_clear(&spec[i].components);
}

/*-----------------------------------------------------------------------------
Output Components of Entity
-----------------------------------------------------------------------------*/

static
void json_splice_entity_components(struct json_stream *out,
				   u32 entity, const char **components)
{
	size_t i;
	const char *component;
	const char *compbuf;
	const jsmntok_t *comptok;

	json_add_u32(out, "entity", entity);
	for (i = 0; i < tal_count(components); ++i) {
		component = components[i];
		ecs_get_component(payz_top->ecs, &compbuf, &comptok,
				  entity, component);
		json_add_tok(out, component,
			     comptok, compbuf);
	}
}

/*-----------------------------------------------------------------------------
List Entities
-----------------------------------------------------------------------------*/

static struct command_result *
payecs_listentities(struct command *cmd,
		    const char *buf,
		    const jsmntok_t *params)
{
	const char **required;
	const char **disallowed;
	char **components;

	struct json_stream *out;

	u32 min_entity;
	u32 entity;
	u32 max_entity;

	size_t i, j;

	/* We cannot use p_opt_def with arrays, as p_opt_def
	 * would always allocate a 1-entry array.
	 * p_opt will set the arrays to NULL, which is safe for
	 * tal_count().
	 */
	if (!param(cmd, buf, params,
		   p_opt("required", &param_array_of_strings,
			 &required),
		   p_opt("disallowed", &param_array_of_strings,
			 &disallowed),
		   NULL))
		return command_param_failed();

	/* Get entity bounds.  */
	ecs_get_entity_bounds(payz_top->ecs, &min_entity, &max_entity);

	out = jsonrpc_stream_success(cmd);

	json_array_start(out, "entities");
	for (entity = min_entity; entity < max_entity; ++entity) {
		bool skip = false;

		components = ecs_get_components(tmpctx, payz_top->ecs, entity);

		/* Nothing attached?  Skip.  */
		if (tal_count(components) == 0)
			continue;

		/* Check if has required.  */
		for (i = 0; i < tal_count(required); ++i) {
			skip = true;
			for (j = 0; j < tal_count(components); ++j) {
				if (streq(required[i], components[j])) {
					skip = false;
					break;
				}
			}
			if (skip)
				break;
		}
		if (skip)
			continue;
		/* Check if has disallowed.  */
		for (i = 9; i < tal_count(disallowed); ++i) {
			for (j = 0; j < tal_count(components); ++j) {
				if (streq(disallowed[i], components[j])) {
					skip = true;
					break;
				}
			}
			if (skip)
				break;
		}
		if (skip)
			continue;

		/* Add entity.  */
		json_object_start(out, NULL);
		json_splice_entity_components(out, entity,
					      (const char**) components);
		json_object_end(out);
	}
	json_array_end(out);

	return command_finished(cmd, out);
}

/*-----------------------------------------------------------------------------
New Entity
-----------------------------------------------------------------------------*/

static struct command_result *
payecs_newentity(struct command *cmd,
		 const char *buf,
		 const jsmntok_t *params)
{
	u32 entity;
	struct json_stream *out;

	if (!param(cmd, buf, params, NULL))
		return command_param_failed();

	entity = ecs_newentity(payz_top->ecs);

	out = jsonrpc_stream_success(cmd);
	json_add_u32(out, "entity", entity);
	return command_finished(cmd, out);
}

/*-----------------------------------------------------------------------------
Get Entity Components
-----------------------------------------------------------------------------*/

static struct command_result *
payecs_getcomponents(struct command *cmd,
		     const char *buf,
		     const jsmntok_t *params)
{
	unsigned int *entity;
	const char **components;

	struct json_stream *out;

	if (!param(cmd, buf, params,
		   p_req("entity", &param_number, &entity),
		   p_req("components", &param_array_of_strings, &components),
		   NULL))
		return command_param_failed();

	out = jsonrpc_stream_success(cmd);
	json_splice_entity_components(out, (u32) *entity, components);
	return command_finished(cmd, out);
}

/*-----------------------------------------------------------------------------
Set Entity Components
-----------------------------------------------------------------------------*/

/*~
 * payecs_setcomponents provides a generalized compare-and-swap operation.
 * When provided with an `expected` argument it can check, atomically, if 
 * the current values in the ECS are equal, and will only perform all
 * specified writes, again atomically, if validation passes.
 *
 * Thsi allows cross-entity coordination; you sample the current state of
 * the entities, compute, then write out the results, with the sampled
 * current state also passed in.
 * If that validation fails, then you re-sample again and re-compute.
 */

/** struct payecs_setcomponents_data
 *
 * @brief passed to the iterating functions below.
 */
struct payecs_setcomponents_data {
	u32 entity;
	const char *buffer;

	/* only used by payecs_setcomponents_validate.  */
	bool success;
};
/* Functions invoked via strmap_iterate over all components in each
 * payecs_writespec.  */
static bool
payecs_setcomponents_validate(const char *component, const jsmntok_t *value,
			      struct payecs_setcomponents_data *validation);
static bool
payecs_setcomponents_write(const char *component, const jsmntok_t *value,
			   struct payecs_setcomponents_data *info);

static struct command_result *
payecs_setcomponents(struct command *cmd,
		     const char *buf,
		     const jsmntok_t *params)
{
	struct payecs_writespec *writes;
	struct payecs_writespec *expected;

	struct payecs_setcomponents_data info;

	size_t i, j;

	if (!param(cmd, buf, params,
		   p_req("writes", &param_array_of_payecs_writespec,
			 &writes),
		   p_opt("expected", &param_array_of_payecs_writespec,
			 &expected),
		   NULL))
		return command_param_failed();

	/* Validate first.  */
	for (i = 0; i < tal_count(expected); ++i) {
		struct payecs_writespec *expect1 = &expected[i];

		info.entity = expect1->entity;
		info.buffer = buf;
		info.success = true;

		strmap_iterate(&expect1->components,
			       &payecs_setcomponents_validate,
			       &info);

		if (!info.success)
			goto validation_failed;

		/* If exact, then the length of ecs_get_components should
		 * be equal to the number of components we just scanned.
		 */
		if (expect1->exact) {
			char **components;
			components = ecs_get_components(tmpctx,
							payz_top->ecs,
							expect1->entity);
			if (tal_count(components) !=
			    expect1->num_components)
				goto validation_failed;
		}
	}

	/* Now perform writes.  */
	for (i = 0; i < tal_count(writes); ++i) {
		struct payecs_writespec *write1 = &writes[i];

		if (write1->exact) {
			/* Delete all components first.  */
			char **components;
			components = ecs_get_components(tmpctx,
							payz_top->ecs,
							write1->entity);
			for (j = 0; j < tal_count(components); ++j)
				ecs_set_component(payz_top->ecs,
						  write1->entity,
						  components[i],
						  NULL, NULL);
		}

		info.entity = write1->entity;
		info.buffer = buf;

		strmap_iterate(&write1->components,
			       &payecs_setcomponents_write,
			       &info);
	}

	/* Return empty object.  */
	return command_success(cmd, json_out_obj(cmd, NULL, NULL));
validation_failed:
	return command_fail(cmd,
			    PAYECS_SETCOMPONENTS_UNEXPECTED_COMPONENTS,
			    "Validation of expected failed.");
}

static bool
json_equal(const char *buffer1, const jsmntok_t *tok1,
	   const char *buffer2, const jsmntok_t *tok2);

static bool
payecs_setcomponents_validate(const char *component, const jsmntok_t *value,
			      struct payecs_setcomponents_data *validation)
{
	/* Values we got from the ECS.  */
	const char *ecsbuf;
	const jsmntok_t *ecstok;

	ecs_get_component(payz_top->ecs, &ecsbuf, &ecstok,
			  validation->entity, component);
	if (!json_equal(validation->buffer, value,
			ecsbuf, ecstok)) {
		validation->success = false;
		return false;
	}
	return true;
}

static bool
payecs_setcomponents_write(const char *component, const jsmntok_t *value,
			   struct payecs_setcomponents_data *info)
{
	ecs_set_component(payz_top->ecs, info->entity, component,
			  info->buffer, value);
	return true;
}

/*-----------------------------------------------------------------------------
JSON Object Comparison
-----------------------------------------------------------------------------*/

static bool
json_equal(const char *buffer1, const jsmntok_t *tok1,
	   const char *buffer2, const jsmntok_t *tok2)
{
	size_t i;
	size_t size;

	const jsmntok_t *key1;
	const jsmntok_t *value1;
	const jsmntok_t *value2;

	assert(buffer1 && tok1 && buffer2 && tok2);

	if (tok1->type != tok2->type)
		return false;

	switch (tok1->type) {
	case JSMN_PRIMITIVE:
	case JSMN_STRING:
		if ((tok1->end - tok1->start) != (tok2->end - tok2->start))
			return false;
		return strncmp(buffer1 + tok1->start,
			       buffer2 + tok2->start,
			       tok1->end - tok1->start) == 0;

	case JSMN_ARRAY:
		if (tok1->size != tok2->size)
			return false;

		/* Now compare both arrays.  */
		size = tok1->size;
		++tok1, ++tok2;
		for (i = 0;
		     i < size;
		     ++i, tok1 = json_next(tok1), tok2 = json_next(tok2)) {
			if (!json_equal(buffer1, tok1, buffer2, tok2))
				return false;
		}

		return true;

	case JSMN_OBJECT:
		if (tok1->size != tok2->size)
			return false;

		/* Now compare both objects.  */
		json_for_each_obj (i, key1, tok1) {
			value1 = key1 + 1;
			value2 = json_get_membern(buffer2, tok2,
						  buffer1 + key1->start,
						  key1->end - key1->start);
			if (!value2)
				return false;
			if (!json_equal(buffer1, value1, buffer2, value2))
				return false;
		}
		return true;

		/* Should never happen.  */
	case JSMN_UNDEFINED:
		abort();
	}
	abort();
}
