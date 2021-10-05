# undef NDEBUG
#include<assert.h>
#include<common/json.h>
#include<common/utils.h>
#include<plugins/payz/tester/tester.h>

int main(int argc, char **argv)
{
	const char *buffer;
	const jsmntok_t *result;
	const jsmntok_t *systems;
	const jsmntok_t *sys;
	const jsmntok_t *sys2;

	size_t i, j;

	bool success;

	payz_tester_init(argv[0]);

	success = payz_tester_command(&buffer, &result,
				      "payecs_getdefaultsystems", "{}");
	assert(success);

	systems = json_get_member(buffer, result, "systems");
	/* Must exist.  */
	assert(systems);

	/* Must be an array of strings.  */
	assert(systems->type == JSMN_ARRAY);
	assert(systems->size > 0);
	json_for_each_arr (i, sys, systems) {
		assert(sys->type == JSMN_STRING);
		/* Every listed system must have a unique name.  */
		json_for_each_arr (j, sys2, systems) {
			if (j >= i)
				break;
			assert(!json_tok_streq(buffer, sys,
					       json_strdup(tmpctx,
							   buffer, sys2)));
		}
	}

	/*~ The exact built-in systems *will* change, so do not hardcode it
	 * here.
	 */

	return 0;
}
