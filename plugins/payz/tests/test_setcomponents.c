#include<plugins/payz/payecs_data.h>
#include<plugins/payz/tester/tester.h>

int main(int argc, char **argv)
{
	payz_tester_init(argv[0]);

	/* Simple no-frills setcomponents.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "[{\"entity\": 1, \"component\": {\"x\": 0}}]",
				   "{}");

	/* Conditional setcomponents.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "[{\"entity\": 1, \"component\": {\"x\": 1}},"
				   " {\"entity\": 1, \"component\": {\"x\": 0}}]",
				   "{}");
	/* Check the conditional setcomponents worked.  */
	payz_tester_command_expect("payecs_getcomponents",
				   "[1, [\"component\"]]",
				   "{\"entity\": 1, \"component\": {\"x\": 1}}");

	/* Failed conditional setcomponents.  */
	payz_tester_command_expectfail("payecs_setcomponents",
				       "[{\"entity\": 1, \"component\": {\"x\": -1}},"
				       " {\"entity\": 1, \"component\": {\"x\": 0}}]",
				       PAYECS_SETCOMPONENTS_UNEXPECTED_COMPONENTS);
	/* Check the conditional setcomponents did not change the entity.  */
	payz_tester_command_expect("payecs_getcomponents",
				   "[1, \"component\"]",
				   "{\"entity\": 1, \"component\": {\"x\": 1}}");

	return 0;
}
