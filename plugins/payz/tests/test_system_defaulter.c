# undef NDEBUG
#include<assert.h>
#include<ccan/array_size/array_size.h>
#include<ccan/str/str.h>
#include<ccan/tal/str/str.h>
#include<common/utils.h>
#include<plugins/payz/tester/tester.h>

/* Expected defaults.  */
struct expected {
	const char *key;
	const char *value;
};

static const struct expected expected[] = {
	{"lightningd:riskfactor", "10"},
	{"lightningd:maxfeepercent", "0.5"},
	{"lightningd:retry_for", "60"},
	{"lightningd:maxdelay", "2016"}, /* Hardcoded by tester framework.  */
	{"lightningd:exemptfee", "\"5000msat\""}
};

#define DUMMY_SYS "test:system_defaulter"

int main(int argc, char **argv)
{
	size_t i;
	char *require;
	bool first;

	payz_tester_init(argv[0]);

	/* Generate the dummy system required components.  */
	require = tal_strdup(tmpctx, "[");
	first = true;
	for (i = 0; i < ARRAY_SIZE(expected); ++i) {
		if (first)
			first = false;
		else
			tal_append_fmt(&require, ", ");
		tal_append_fmt(&require, "\"%s\"", expected[i].key);
	}
	tal_append_fmt(&require, "]");
	/* Register the dummy system.  */
	payz_tester_command_ok("payecs_newsystem",
			       tal_fmt(tmpctx, "[\""DUMMY_SYS"\", %s]",
				       require));

	/* Set up an entity.  */
	payz_tester_command_ok("payecs_setdefaultsystems",
			       "[42, [\""DUMMY_SYS"\"]]");
	payz_tester_command_ok("payecs_setcomponents",
			       "[{\"entity\": 42, "
			       "\"lightningd:main-payment\": true}]");

	/* Run it.  */
	payz_tester_command_ok("payecs_advance", "[42]");

	/* Now check that default values appear.  */
	for (i = 0; i < ARRAY_SIZE(expected); ++i) {
		const char *buf;
		const jsmntok_t *res;
		char *res_str;

		payz_tester_wait_component(&buf, &res,
					   42, expected[i].key);
		res_str = tal_strndup(tmpctx,
				      json_tok_full(buf, res),
				      json_tok_full_len(res));
		assert(streq(res_str, expected[i].value));
	}

	return 0;
}
