#include"parse_invoice.h"
#include<ccan/tal/str/str.h>
#include<common/errcode.h>
#include<common/jsonrpc_errors.h>
#include<common/json.h>
#include<common/json_stream.h>
#include<common/utils.h>
#include<plugins/libplugin.h>

static struct command_result *
parse_invoice(struct ecs *, struct command *,
	      u32 entity,
	      const char *buffer, const jsmntok_t *eo);
static struct command_result *
promote_invoice_type(struct ecs *ecs, struct command *cmd,
		     u32 entity,
		     const char *buffer, const jsmntok_t *eo);

struct ecs_register_desc system_parse_invoice[] = {
	ECS_REGISTER_NAME("lightningd:parse_invoice"),
	ECS_REGISTER_FUNC(&parse_invoice),
	ECS_REGISTER_REQUIRE("lightningd:invoice"),
	ECS_REGISTER_DISALLOW("lightningd:parse_invoice:ran"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("lightningd:promote_invoice_type"),
	ECS_REGISTER_FUNC(&promote_invoice_type),
	ECS_REGISTER_REQUIRE("lightningd:invoice:type"),
	ECS_REGISTER_DISALLOW("lightningd:promote_invoice_type:ran"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_OVER_AND_OUT()
};

/*-----------------------------------------------------------------------------
Invoice Parsing
-----------------------------------------------------------------------------*/

struct parse_invoice_closure {
	struct ecs *ecs;
	struct command *cmd;
	u32 entity;
};

static struct command_result *
parse_invoice_decode_ok(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *res,
			 struct parse_invoice_closure *closure);
static struct command_result *
parse_invoice_decode_ng(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *res,
			 struct parse_invoice_closure *closure);

static struct command_result *
parse_invoice(struct ecs *ecs, struct command *cmd,
              u32 entity,
              const char *buffer, const jsmntok_t *eo)
{
	const jsmntok_t *invoice;

	struct parse_invoice_closure *closure;
	struct out_req *req;

	invoice = json_get_member(buffer, eo, "lightningd:invoice");

	/* Prevent ourselves from running again.  */
	ecs_set_component_datum(ecs, entity,
				"lightningd:parse_invoice:ran", "true");

	/* Keep our needed variables around.  */
	closure = tal(cmd, struct parse_invoice_closure);
	closure->ecs = ecs;
	closure->cmd = cmd;
	closure->entity = entity;

	req = jsonrpc_request_start(cmd->plugin, cmd, "decode",
				    &parse_invoice_decode_ok,
				    &parse_invoice_decode_ng,
				    closure);
	json_add_tok(req->js, "string", invoice, buffer);
	return send_outreq(cmd->plugin, req);
}

static struct command_result *
parse_invoice_decode_ok(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *res,
			 struct parse_invoice_closure *closure)
{
	const jsmntok_t *key;
	char *key_string;
	const jsmntok_t *value;
	size_t i;

	tal_steal(tmpctx, closure);

	json_for_each_obj(i, key, res) {
		value = key + 1;

		key_string = tal_fmt(tmpctx, "lightningd:invoice:%.*s",
				     /* Get string contents and not include
				      * the \" delimiters, so cannot use
				      * json_tok_full.
				      */
				     key->end - key->start,
				     buf + key->start);

		ecs_set_component(closure->ecs, closure->entity,
				  key_string,
				  buf, value);
	}

	return ecs_advance_done(cmd, closure->ecs, closure->entity);
}
static struct command_result *
parse_invoice_decode_ng(struct command *cmd,
			 const char *buf,
			 const jsmntok_t *res,
			 struct parse_invoice_closure *closure)
{
	tal_steal(tmpctx, closure);

	ecs_set_component(closure->ecs, closure->entity,
			  "lightningd:error", buf, res);
	ecs_set_component(closure->ecs, closure->entity,
			  "lightningd:systems", NULL, NULL);

	return ecs_done(cmd, closure->ecs);
}

/*-----------------------------------------------------------------------------
Invoice Type Promotion
-----------------------------------------------------------------------------*/

/*~
 * The `decode` command also returns a `type` field containing a
 * string type for the input string.
 * Promote it to a flag-type Component: for example if the type
 * is `"bolt11 invoice"` attach a Component with the name
 * `lightningd:invoice:type:bolt11 invoice`.
 */

static struct command_result *
promote_invoice_type(struct ecs *ecs, struct command *cmd,
		     u32 entity,
		     const char *buffer, const jsmntok_t *eo)
{
	const jsmntok_t *type_tok;
	char *type;

	type_tok = json_get_member(buffer, eo, "lightningd:invoice:type");
	type = json_strdup(tmpctx, buffer, type_tok);

	ecs_set_component_datum(ecs, entity,
				tal_fmt(tmpctx, "lightningd:invoice:type:%s",
					type),
				"true");

	return ecs_advance_done(cmd, ecs, entity);
}
