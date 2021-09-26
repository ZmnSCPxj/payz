#ifndef LIGHTNING_PLUGINS_PAYZ_SETSYSTEMS_H
#define LIGHTNING_PLUGINS_PAYZ_SETSYSTEMS_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<ccan/typesafe_cb/typesafe_cb.h>
#include<common/json.h>
#include<stddef.h>

/** payz_setsystems_tok
 *
 * @brief Set a field of the `lightningd:systems` component.
 * 
 * @desc By convention this component is an object.
 * If not attached, or attached to a non-object, replaces
 * it with an object, otherwise it creates a copy of the
 * object with the specified field replaced, removed, or
 * added.
 *
 * @param entity - the Entity to modify.
 * @param fieldname - the field of `lightningd:systems` to
 * modify, add, or remove.
 * @param buffer - the buffer containing the JSON text.
 * Can be NULL to remove the field instead.
 * @param toks - an array of JSMN tokens of the parsed
 * JSON text.
 * If buffer is NULL, should be NULL as well
 */
void payz_setsystems_tok(u32 entity,
			 const char *fieldname,
			 const char *buffer,
			 const jsmntok_t *toks);

/** payz_setsystems
 *
 * @brief Set a field of the `lightningd:systems` component.
 * 
 * @desc By convention this component is an object.
 * If not attached, or attached to a non-object, replaces
 * it with an object, otherwise it creates a copy of the
 * object with the specified field replaced, removed, or
 * added.
 *
 * @param entity - the Entity to modify.
 * @param fieldname - the field of `lightningd:systems` to
 * modify, add, or remove.
 * @param value - a nul-terminated C string containing valid
 * JSON text.
 */
void payz_setsystems(u32 entity,
		     const char *fieldname,
		     const char *value);

/** payz_generic_setsystems_tok
 *
 * @brief Like payz_setsystems_tok, but gets functions and object
 * to use from the caller instead of from payz_top.
 *
 * @desc This is the real core function.
 *
 * @param get_component - function to get a component from
 * the specified Entity-Component table.
 * @param set_component - function to set a component in
 * the specified Entity-Component table.
 * @param ec - the Entity-Component table.
 * @param entity - the Entity to modify.
 * @param fieldname - the field of `lightningd:systems` to
 * modify, add, or remove.
 * @param buffer - the buffer containing the JSON text.
 * Can be NULL to remove the field instead.
 * @param toks - an array of JSMN tokens of the parsed
 * JSON text.
 * If buffer is NULL, should be NULL as well
 */
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
				 const char *buffer,
				 const jsmntok_t *toks);

/** payz_getsystems
 *
 * @brief Get a field of `lightningd:systems`, or return
 * false on failure.
 *
 * @param entity - the Entity to query.
 * @param fieldname - the field of `lightningd:systems`.
 * @param json_to_x - the json_to_* function to use to
 * parse the field.
 * @param variable - the location to pass to the json_to_x
 * function.
 *
 * @return - true if the field exists *and* parsing of
 * json_to_x succeeded.
 */
bool payz_getsystems_(u32 entity,
		      const char *fieldname,
		      bool (*json_to_x)(const char *buffer,
					const jsmntok_t *tok,
					void *variable),
		      void *variable);
#define payz_getsystems(e, f, j, v) \
	payz_getsystems_((e), (f), \
			 typesafe_cb_preargs(bool, void*, \
					     (j), (v), \
					     const char *, \
					     const jsmntok_t *), \
			 (v))
/** payz_getsystems_tal
 *
 * @brief Like payz_getsystems but using a tal-allocating JSON
 * parsing function.
 *
 * @param ctx - the owenr of the result.
 * @param entity - the Entity to query.
 * @param fieldname - the field of `lightningd:systems`.
 * @param json_to_x - the json_to_* function to use to
 * parse the field.
 * @param variable - the location to set with the result
 * from the json_to_x function.
 *
 * @return - true if the field exists *and* parsing of
 * json_to_x succeeded.
 */
bool payz_getsystems_tal_(const tal_t *ctx,
			  u32 entity,
			  const char *fieldname,
			  void *(*json_to_x)(const tal_t *ctx,
					     const char *buffer,
					     const jsmntok_t *tok),
			  void **variable);
#define payz_getsystems_tal(ctx, e, f, j, v) \
	payz_getsystems_tal_((ctx), (e), (f), \
			     ((void *(*)(const tal_t *, \
					 const char *, \
					 const jsmntok_t *)) (j)), \
			     ((void **) \
			      ((v) + 0 * sizeof((*(v)) = (j)((ctx), \
							     (const char *) NULL,\
							     (const jsmntok_t *) NULL)))))

/** payz_generic_getsystems
 *
 * @brief like payz_getsystems, but uses the Entity Component
 * table passed in.
 *
 * @param get_component - function to get the component from the
 * given Entity Component table.
 * @param ec - the Entity Component table.
 * @param entity - the Entity to query.
 * @param fieldname - the field of `lightningd:systems`.
 * @param json_to_x - the json_to_* function to use to
 * parse the field.
 * @param variable - the location to pass to the json_to_x
 * function.
 *
 * @return - true if the field exists *and* parsing of
 * json_to_x succeeded.
 */
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
			      void *variable);
#define payz_generic_getsystems(ge, ec, e, f, j, v) \
	payz_generic_getsystems_((ge), (ec), (e), (f), \
				 typesafe_cb_preargs(bool, void*, \
						     (j), (v), \
						     const char *, \
						     const jsmntok_t *), \
				 (v))

/** payz_generic_getsystems_tal
 *
 * @brief Like payz_generic_getsystems, but using a tal-allocating
 * JSON parsing function.
 *
 * @param ctx - the owenr of the result.
 * @param get_component - function to get the component from the
 * given Entity Component table.
 * @param ec - the Entity Component table.
 * @param entity - the Entity to query.
 * @param fieldname - the field of `lightningd:systems`.
 * @param json_to_x - the json_to_* function to use to
 * parse the field.
 * @param variable - the location to set with the result
 * from the json_to_x function.
 *
 * @return - true if the field exists *and* parsing of
 * json_to_x succeeded.
 */
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
				  void **variable);
#define payz_generic_getsystems_tal(ctx, ge, ec, e, f, j, v) \
	payz_generic_getsystems_tal_((ctx), (ge), (ec), (e), (f), \
				     ((void *(*)(const tal_t *, \
						 const char *, \
						 const jsmntok_t *)) (j)), \
				     ((void **) \
				      ((v) + 0 * sizeof((*(v)) = (j)((ctx), \
								     (const char *) NULL,\
								     (const jsmntok_t *) NULL)))))

#endif /* LIGHTNING_PLUGINS_PAYZ_SETSYSTEMS_H */
