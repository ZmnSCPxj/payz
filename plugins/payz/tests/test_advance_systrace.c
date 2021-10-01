# undef NDEBUG
#include<assert.h>
#include<ccan/err/err.h>
#include<ccan/short_types/short_types.h>
#include<ccan/str/str.h>
#include<common/json.h>
#include<plugins/payz/ecs/ecsys.h>
#include<plugins/payz/tester/tester.h>

#define DUMMY_SYS "payz:tests:test_advance_systrace"

int main(int argc, char **argv)
{
	const char *buffer;
	const jsmntok_t *result;
	const char *error;

	char *tmp;

	u32 entity1, entity2;
	char *system;
	u32 example;

	payz_tester_init(argv[0]);

	tmp = tal(NULL, char);

	/**
	 * Test program for payecs_advance and payecs_systrace.
	 */

	/* Register the dummy system.  */
	payz_tester_command_expect("payecs_newsystem",
				   "[\""DUMMY_SYS"\", [\"example\"]]",
				   "{}");
	/* Add the dummy system to Entity 1.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "[{\"entity\": 1, \"lightningd:systems\": {"
				   "\"systems\": [\""DUMMY_SYS"\"]}}]",
				   "{}");

	/* Advancing should still fail with "cannot advance" since the entity
	 * has no "example" component.
	 */
	payz_tester_command_expectfail("payecs_advance", "[1]",
				       PAY_ECS_NOT_ADVANCEABLE);
	/* And systrace should still remain empty.  */
	payz_tester_command_expect("payecs_systrace", "[1]",
				   "{\"entity\": 1, \"trace\": []}");
	payz_tester_command_expect("payecs_systrace", "[2]",
				   "{\"entity\": 2, \"trace\": []}");

	/* Now add the required component.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "[{\"entity\": 1, \"example\": 42}]",
				   "{}");
	/* Advancing should now succeed.  */
	payz_tester_command_expect("payecs_advance", "[1]", "{}");

	/* And the dummy system should get traced.  */
	payz_tester_command(&buffer, &result,
			    "payecs_systrace", "[1]");
	error = json_scan(tmp, buffer, result,
			  "{entity:%,"
			  "trace:[0:{system:%,"
			  "entity:{entity:%,example:%}}]}",
			  JSON_SCAN(json_to_u32, &entity1),
			  JSON_SCAN_TAL(tmp, json_strdup, &system),
			  JSON_SCAN(json_to_u32, &entity2),
			  JSON_SCAN(json_to_u32, &example));
	if (error)
		errx(1,
		     "payecs_systrace result: %s: %*.s",
		     error,
		     json_tok_full_len(result),
		     json_tok_full(buffer, result));
	assert(entity1 == entity2);
	assert(entity1 == 1);
	assert(streq(system, DUMMY_SYS));
	assert(example == 42);

	/* Systrace of other entities should remain empty.  */
	payz_tester_command_expect("payecs_systrace", "[2]",
				   "{\"entity\": 2, \"trace\": []}");

	tal_free(tmp);

	return 0;
}
