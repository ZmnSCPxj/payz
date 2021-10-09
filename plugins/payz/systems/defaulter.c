#include"defaulter.h"
#include<assert.h>
#include<plugins/libplugin.h>

/* Contains systems that handle default settings for *main* payment
 * entities.
 *
 * Why make them systems instead of just leaving it to `pay` command
 * to fill in?
 *
 * Because plugins that want custom payment flows may want to be
 * prototyped with as little code as needed, meaning they want to
 * fill in the defaults as much as possible.
 * By making the defaults systems, custom payment plugins can
 * simply not expose these settings (at least while prototyping)
 * and use the defaults.
 * Without these systems, custom payment plugins need to fill in
 * these settings themselves.
 * *With* these settings, custom payment plugins can at least
 * *start* being developed with using only the default values at
 * all times, but can later override these values if needed.
 */

/* Below are the default settings, as C strings containing
 * JSON datums.
 */
static const char *default_riskfactor = "10";
static const char *default_maxfeepercent = "0.5";
static const char *default_retry_for = "60";
static const char *default_exemptfee = "\"5000msat\"";

/* We get this at init time.  */
static char *default_maxdelay = NULL;

static struct command_result *
defaulter_riskfactor(struct ecs *ecs,
		     struct command *cmd,
		     u32 entity,
		     const char *buffer,
		     const jsmntok_t *components);
static struct command_result *
defaulter_maxfeepercent(struct ecs *ecs,
			struct command *cmd,
			u32 entity,
			const char *buffer,
			const jsmntok_t *components);
static struct command_result *
defaulter_retry_for(struct ecs *ecs,
		    struct command *cmd,
		    u32 entity,
		    const char *buffer,
		    const jsmntok_t *components);
static struct command_result *
defaulter_maxdelay(struct ecs *ecs,
		   struct command *cmd,
		   u32 entity,
		   const char *buffer,
		   const jsmntok_t *components);
static struct command_result *
defaulter_exemptfee(struct ecs *ecs,
		    struct command *cmd,
		    u32 entity,
		    const char *buffer,
		    const jsmntok_t *components);

struct ecs_register_desc system_defaulter[] = {
	ECS_REGISTER_NAME("lightningd:default_riskfactor"),
	ECS_REGISTER_FUNC(&defaulter_riskfactor),
	ECS_REGISTER_REQUIRE("lightningd:main-payment"),
	ECS_REGISTER_DISALLOW("lightningd:riskfactor"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("lightningd:default_maxfeepercent"),
	ECS_REGISTER_FUNC(&defaulter_maxfeepercent),
	ECS_REGISTER_REQUIRE("lightningd:main-payment"),
	ECS_REGISTER_DISALLOW("lightningd:maxfeepercent"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("lightningd:default_retry_for"),
	ECS_REGISTER_FUNC(&defaulter_retry_for),
	ECS_REGISTER_REQUIRE("lightningd:main-payment"),
	ECS_REGISTER_DISALLOW("lightningd:retry_for"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("lightningd:default_maxdelay"),
	ECS_REGISTER_FUNC(&defaulter_maxdelay),
	ECS_REGISTER_REQUIRE("lightningd:main-payment"),
	ECS_REGISTER_DISALLOW("lightningd:maxdelay"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_NAME("lightningd:default_exemptfee"),
	ECS_REGISTER_FUNC(&defaulter_exemptfee),
	ECS_REGISTER_REQUIRE("lightningd:main-payment"),
	ECS_REGISTER_DISALLOW("lightningd:exemptfee"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_OVER_AND_OUT()
};

void system_defaulter_init(struct plugin *plugin)
{
	rpc_scan(plugin, "listconfigs",
		 take(json_out_obj(NULL, NULL, NULL)),
		 "{max-locktime-blocks:%}",
		 JSON_SCAN_TAL(plugin, json_strdup, &default_maxdelay));
}

static struct command_result *
defaulter_riskfactor(struct ecs *ecs,
		     struct command *cmd,
		     u32 entity,
		     const char *buffer,
		     const jsmntok_t *components)
{
	ecs_set_component_datum(ecs, entity,
				"lightningd:riskfactor",
				default_riskfactor);
	return ecs_advance_done(cmd, ecs, entity);
}

static struct command_result *
defaulter_maxfeepercent(struct ecs *ecs,
			struct command *cmd,
			u32 entity,
			const char *buffer,
			const jsmntok_t *components)
{
	ecs_set_component_datum(ecs, entity,
				"lightningd:maxfeepercent",
				default_maxfeepercent);
	return ecs_advance_done(cmd, ecs, entity);
}

static struct command_result *
defaulter_retry_for(struct ecs *ecs,
		    struct command *cmd,
		    u32 entity,
		    const char *buffer,
		    const jsmntok_t *components)
{
	ecs_set_component_datum(ecs, entity,
				"lightningd:retry_for",
				default_retry_for);
	return ecs_advance_done(cmd, ecs, entity);
}

static struct command_result *
defaulter_exemptfee(struct ecs *ecs,
		    struct command *cmd,
		    u32 entity,
		    const char *buffer,
		    const jsmntok_t *components)
{
	ecs_set_component_datum(ecs, entity,
				"lightningd:exemptfee",
				default_exemptfee);
	return ecs_advance_done(cmd, ecs, entity);
}

static struct command_result *
defaulter_maxdelay(struct ecs *ecs,
		   struct command *cmd,
		   u32 entity,
		   const char *buffer,
		   const jsmntok_t *components)
{
	assert(default_maxdelay);
	ecs_set_component_datum(ecs, entity,
				"lightningd:maxdelay",
				default_maxdelay);
	return ecs_advance_done(cmd, ecs, entity);
}
