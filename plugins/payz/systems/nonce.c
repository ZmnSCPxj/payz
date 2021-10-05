#include"nonce.h"
#include<assert.h>
#include<ccan/tal/str/str.h>
#include<common/utils.h>
#include<plugins/libplugin.h>
#include<sodium/randombytes.h>

/** lightningd:generate_nonce
 *
 * @brief Generates a random 32-byte nonce for each entity.
 */

static void generate_nonce(struct ecs *ecs,
			   struct command *cmd,
			   u32 entity,
			   const char *buffer,
			   const jsmntok_t *eo);

struct ecs_register_desc system_nonce[] = {
	ECS_REGISTER_NAME("lightningd:generate_nonce"),
	ECS_REGISTER_FUNC(&generate_nonce),
	ECS_REGISTER_REQUIRE("lightningd:systems"), /* Dummy.  */
	ECS_REGISTER_DISALLOW("lightningd:nonce"),
	ECS_REGISTER_DONE(),

	ECS_REGISTER_OVER_AND_OUT()
};
static void generate_nonce(struct ecs *ecs,
			   struct command *cmd,
			   u32 entity,
			   const char *buffer,
			   const jsmntok_t *eo)
{
	u8 nonce[32];
	char *nonce_hex;

	randombytes_buf(nonce, sizeof(nonce));
	nonce_hex = tal_hexstr(tmpctx, nonce, sizeof(nonce));

	ecs_set_component_datum(ecs, entity, "lightningd:nonce",
				tal_fmt(tmpctx, "\"%s\"", nonce_hex));

	ecs_advance_done(cmd->plugin, ecs, entity);
}
