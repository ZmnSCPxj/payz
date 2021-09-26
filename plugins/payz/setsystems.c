#include"setsystems.h"
#include<ccan/json_out/json_out.h>
#include<common/json_stream.h>
#include<common/utils.h>
#include<plugins/payz/ecs/ecs.h>
#include<plugins/payz/top.h>

static const char *syscomp = "lightningd:systems";

void payz_generic_setsystems_tok(bool (*get_component)(const void *ec,
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
				 u32 entity,
				 const char *fieldname,
				 const char *value_buffer,
				 const jsmntok_t *value_toks)
{
	struct json_stream *js;

	const char *buffer;
	const jsmntok_t *toks;

	size_t i;
	const jsmntok_t *key;
	const jsmntok_t *value;

	const char *ncomp_buffer;
	size_t ncomp_buffer_len;
	jsmntok_t *ncomp_toks;

	js = new_json_stream(tmpctx, NULL, NULL);
	json_object_start(js, NULL);

	/* Copy fields from existing object, if existing component is
	 * an object.  */
	(void) get_component(ec, &buffer, &toks, entity, syscomp);
	if (toks->type == JSMN_OBJECT) {
		json_for_each_obj (i, key, toks) {
			/* Skip the field being modified.  */
			if (json_tok_streq(buffer, key, fieldname))
				continue;

			value = key + 1;

			json_add_tok(js, json_strdup(tmpctx, buffer, key),
				     value, buffer);
		}
	}

	/* Write the new field if not C NULL.  */
	if (value_buffer)
		json_add_tok(js, fieldname, value_toks, value_buffer);

	json_object_end(js);

	/* Extract the created object.  */
	ncomp_buffer = json_out_contents(js->jout, &ncomp_buffer_len);
	ncomp_toks = json_parse_simple(tmpctx,
				       ncomp_buffer, ncomp_buffer_len);

	/* Finally set the component.  */
	set_component(ec, entity, syscomp, ncomp_buffer, ncomp_toks);
}

void payz_setsystems_tok(u32 entity,
			 const char *fieldname,
			 const char *buffer,
			 const jsmntok_t *toks)
{
	typedef bool (*get_component_t)(const void *,
					const char **,
					const jsmntok_t **,
					u32,
					const char *);
	typedef void (*set_component_t)(void *,
					u32,
					const char *,
					const char *,
					const jsmntok_t *);
	payz_generic_setsystems_tok((get_component_t) &ecs_get_component,
				    (set_component_t) &ecs_set_component,
				    payz_top->ecs,
				    entity, fieldname,
				    buffer, toks);
}

void payz_setsystems(u32 entity,
		     const char *fieldname,
		     const char *value)
{
	jsmntok_t *toks;

	toks = json_parse_simple(tmpctx, value, strlen(value));

	payz_setsystems_tok(entity, fieldname, value, toks);
}

bool payz_generic_getsystems_(bool (*get_entity)(const void *ec,
						 const char **buffer,
						 const jsmntok_t **toks,
						 u32 entity,
						 const char *component),
			      const void *ec,
			      u32 entity,
			      const char *fieldname,
			      bool (*json_to_x)(const char *buffer,
						const jsmntok_t *tok,
						void *variable),
			      void *variable)
{
	const char *buffer;
	const jsmntok_t *toks;

	if (!get_entity(ec, &buffer, &toks, entity, syscomp))
		return false;

	toks = json_get_member(buffer, toks, fieldname);
	if (!toks)
		return false;

	return json_to_x(buffer, toks, variable);
}

bool payz_generic_getsystems_tal_(const tal_t *ctx,
				  bool (*get_entity)(const void *ec,
						     const char **buffer,
						     const jsmntok_t **toks,
						     u32 entity,
						     const char *component),
				  const void *ec,
				  u32 entity,
				  const char *fieldname,
				  void *(*json_to_x)(const tal_t *ctx,
						     const char *buffer,
						     const jsmntok_t *tok),
				  void **variable)
{
	const char *buffer;
	const jsmntok_t *toks;

	if (!get_entity(ec, &buffer, &toks, entity, syscomp))
		return false;

	toks = json_get_member(buffer, toks, fieldname);
	if (!toks)
		return false;

	*variable = json_to_x(ctx, buffer, toks);
	if (!*variable)
		return false;

	return true;
}

bool payz_getsystems_(u32 entity,
		      const char *fieldname,
		      bool (*json_to_x)(const char *buffer,
					const jsmntok_t *tok,
					void *variable),
		      void *variable)
{
	typedef bool (*get_component_t)(const void *,
					const char **,
					const jsmntok_t **,
					u32,
					const char *);
	return payz_generic_getsystems_((get_component_t) &ecs_get_component,
					payz_top->ecs,
					entity, fieldname,
					json_to_x, variable);
}

bool payz_getsystems_tal_(const tal_t *ctx,
			  u32 entity,
			  const char *fieldname,
			  void *(*json_to_x)(const tal_t *ctx,
					     const char *buffer,
					     const jsmntok_t *tok),
			  void **variable)
{
	typedef bool (*get_component_t)(const void *,
					const char **,
					const jsmntok_t **,
					u32,
					const char *);
	return payz_generic_getsystems_tal_(ctx,
					    (get_component_t) &ecs_get_component,
					    payz_top->ecs,
					    entity, fieldname,
					    json_to_x, variable);
}
