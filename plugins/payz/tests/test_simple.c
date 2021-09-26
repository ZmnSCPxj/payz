#include<plugins/payz/tester/tester.h>

int main(int argc, char **argv)
{
	payz_tester_init(argv[0]);

	/* New entity should start at 1 and increment.  */
	payz_tester_command_expect("payecs_newentity", "{}", "{\"entity\": 1}");
	payz_tester_command_expect("payecs_newentity", "{}", "{\"entity\": 2}");
	payz_tester_command_expect("payecs_newentity", "{}", "{\"entity\": 3}");

	/* No entities in the entity-component table at start.  */
	payz_tester_command_expect("payecs_listentities", "{}",
				   "{\"entities\": []}");
	/* Add an entity.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "{\"writes\": [{\"entity\": 1, \"example\": 42}]}",
				   "{}");
	payz_tester_command_expect("payecs_listentities", "{}",
				   "{\"entities\": [{\"entity\": 1, \"example\": 42}]}");

	/* Now delete it.  */
	payz_tester_command_expect("payecs_setcomponents",
				   "{\"writes\": [{\"entity\": 1, \"example\": null}]}",
				   "{}");
	payz_tester_command_expect("payecs_listentities", "{}",
				   "{\"entities\": []}");

	return 0;
}
