#include<plugins/payz/ecs/ecsys.h>
#include<plugins/payz/tester/tester.h>

int main(int argc, char **argv)
{
	payz_tester_init(argv[0]);

	/**
	 * Test program for payecs_advance failures.
	 */

	/* Attempt to advance an entity without a
	 * lightningd:systems.
	 */
	payz_tester_command_expectfail("payecs_advance", "[1]",
				       PAY_ECS_INVALID_SYSTEMS_COMPONENT);

	/* Set lightningd:systems without a `systems` field.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "[{\"entity\": 1,"
				   "\"lightningd:systems\": {\"foo\": 42}}]",
				   "{}");
	/* Should still fail.  */
	payz_tester_command_expectfail("payecs_advance", "[1]",
				       PAY_ECS_INVALID_SYSTEMS_COMPONENT);

	/* Set lightningd:systems with a systems that is an empty array.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "[{\"entity\": 1, "
				   "  \"lightningd:systems\": {\"systems\": []}}]",
				   "{}");
	/* Should fail with a different error code.  */
	payz_tester_command_expectfail("payecs_advance", "[1]",
				       PAY_ECS_NOT_ADVANCEABLE);

	return 0;
}
