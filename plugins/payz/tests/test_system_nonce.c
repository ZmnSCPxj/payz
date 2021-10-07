# undef NDEBUG
#include<assert.h>
#include<ccan/str/str.h>
#include<common/json.h>
#include<common/utils.h>
#include<plugins/payz/tester/tester.h>

/* "S"ystem "U"nder "T"est.  */
#define SUT "lightningd:generate_nonce"
#define DUMMY_SYS "test:system_nonce:dummy"

int main (int argc, char **argv)
{
	bool found;
	bool ret;

	const char *buffer;
	const jsmntok_t *result;
	const jsmntok_t *nonce;
	const jsmntok_t *trace;

	payz_tester_init(argv[0]);

	/**
	 * Test program for systems/nonce.c.
	 */

	/* Register a dummy system.  */
	payz_tester_command_ok("payecs_newsystem",
			       "[\""DUMMY_SYS"\", [\"lightningd:nonce\"]]");

	/* Add the generate_nonce and dummy system to an arbitrary entity.  */
	payz_tester_command_ok("payecs_setcomponents",
			       "[{\"entity\": 42, \"lightningd:systems\": "
			       "{\"systems\": [\""SUT"\", \""DUMMY_SYS"\"]}}]");

	/* Advance the entity, it should work fine.  */
	payz_tester_command_ok("payecs_advance", "[42]");

	/* Wait for lightningd:nonce component to appear (race condition with
	 * the plugin process!).
	 */
	payz_tester_wait_component(&buffer, &nonce,
				   42, "lightningd:nonce");
	/* Should be a 64-char hex string.  */
	assert(nonce->type == JSMN_STRING);
	assert(nonce->end - nonce->start == 64);
	assert(json_tok_bin_from_hex(tmpctx, buffer, nonce));

	/* Wait for payecs_systrace to trace two systems -- the system
	 * under test and the dummy system --- on the entity.
	 */
	found = false;
	while (!found) {
		ret = payz_tester_command(&buffer, &result,
					  "payecs_systrace", "[42]");
		assert(ret);

		trace = json_get_member(buffer, result, "trace");
		assert(trace);

		assert(trace->type == JSMN_ARRAY);

		/* Not length 2 yet?  Try again.  */
		if (trace->size < 2)
			continue;

		found = true;
	}
	/* Should trace two systems.  */
	assert(json_get_member(buffer, json_get_arr(trace, 0), "system"));
	assert(json_tok_streq(buffer, json_get_member(buffer, json_get_arr(trace, 0), "system"), SUT));
	assert(json_get_member(buffer, json_get_arr(trace, 1), "system"));
	assert(json_tok_streq(buffer, json_get_member(buffer, json_get_arr(trace, 1), "system"), DUMMY_SYS));

	return 0;
}
