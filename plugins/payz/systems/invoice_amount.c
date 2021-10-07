#include"invoice_amount.h"
#include<ccan/json_out/json_out.h>
#include<common/json_stream.h>
#include<common/jsonrpc_errors.h>
#include<common/utils.h>
#include<plugins/libplugin.h>

static void invoice_amount_msat(struct ecs *ecs, struct command *cmd,
				u32 entity,
				const char *buffer,
				const jsmntok_t *components);
static void fail_invoice_amount_and_amount(struct ecs *ecs,
					   struct command *cmd,
					   u32 entity,
					   const char *buffer,
					   const jsmntok_t *components);

struct ecs_register_desc system_invoice_amount[] = {
	ECS_REGISTER_NAME("lightningd:invoice_amount_msat"),
	ECS_REGISTER_FUNC(&invoice_amount_msat),
	ECS_REGISTER_REQUIRE("lightningd:invoice:amount_msat"),
	ECS_REGISTER_DISALLOW("lightningd:amount"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("lightningd:fail_invoice_amount_and_amount"),
	ECS_REGISTER_FUNC(&fail_invoice_amount_and_amount),
	ECS_REGISTER_REQUIRE("lightningd:invoice:amount_msat"),
	ECS_REGISTER_REQUIRE("lightningd:amount"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_OVER_AND_OUT()
};

static void invoice_amount_msat(struct ecs *ecs, struct command *cmd,
				u32 entity,
				const char *buffer,
				const jsmntok_t *components)
{
	const jsmntok_t *amount_msat;

	amount_msat = json_get_member(buffer, components,
				      "lightningd:invoice:amount_msat");
	if (!amount_msat) {
		plugin_log(cmd->plugin, LOG_UNUSUAL,
			   "lightningd:invoice_amount_msat triggered "
			   "without required components? %.*s",
			   json_tok_full_len(components),
			   json_tok_full(buffer, components));
		return;
	}

	/* Detach lightningd:invoice:amount_msat, attach lightningd:amount.  */
	ecs_set_component_datum(ecs, entity, "lightningd:invoice:amount_msat",
				NULL);
	ecs_set_component(ecs, entity, "lightningd:amount", buffer, amount_msat);

	ecs_advance_done(cmd->plugin, ecs, entity);
}

static void fail_invoice_amount_and_amount(struct ecs *ecs,
					   struct command *cmd,
					   u32 entity,
					   const char *buffer,
					   const jsmntok_t *components)
{
	struct json_stream *js;

	const char *datum;
	size_t len;

	/* The user specified an amount, but the invoice has its own amount.
	 * Generate an error object.
	 */
	js = new_json_stream(tmpctx, NULL, NULL);
	json_object_start(js, NULL);
	json_add_errcode(js, "code", JSONRPC2_INVALID_PARAMS);
	json_add_string(js, "message", "msatoshi parameter unnecessary");
	json_object_end(js);

	datum = json_out_contents(js->jout, &len);
	ecs_set_component_datuml(ecs, entity, "lightningd:error", datum, len);

	/* Stop processing.  */
	ecs_set_component_datum(ecs, entity, "lightningd:systems", NULL);
}
