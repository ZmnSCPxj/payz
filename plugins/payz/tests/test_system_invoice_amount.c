# undef NDEBUG
#include<assert.h>
#include<plugins/payz/tester/tester.h>

#define DUMMY_SYS "test:invoice_amount"
#define TEST_AMOUNT "123456789msat"

int main(int argc, char **argv)
{
	const char *buffer;
	const jsmntok_t *amount;

	payz_tester_init(argv[0]);

	/* Register the dummy system to trigger on lightningd:amount.  */
	payz_tester_command_ok("payecs_newsystem",
			       "[\""DUMMY_SYS"\", [\"lightningd:amount\"]]");

	/* Set up an Entity.  */
	payz_tester_command_ok("payecs_setcomponents",
			       "[{\"entity\": 42, "
			       "\"lightningd:invoice:amount_msat\": \""TEST_AMOUNT"\"}]");
	payz_tester_command_ok("payecs_setdefaultsystems",
			       "[42, [\""DUMMY_SYS"\"]]");
	/* Double-check it has lightningd:invoice:amount_msat.  */
	payz_tester_wait_component(&buffer, &amount,
				   42, "lightningd:invoice:amount_msat");

	/* Advance it.  */
	payz_tester_command_ok("payecs_advance", "[42]");

	/* Check that lightningd:invoice:amount_msat has disappeared and
	 * we now have lightningd:amount.  */
	payz_tester_wait_detach_component(42, "lightningd:invoice:amount_msat");
	payz_tester_wait_component(&buffer, &amount,
				   42, "lightningd:amount");
	assert(json_tok_streq(buffer, amount, TEST_AMOUNT));

	return 0;
}
