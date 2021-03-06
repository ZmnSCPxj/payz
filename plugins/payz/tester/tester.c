#include"tester.h"
#include<assert.h>
#include<ccan/err/err.h>
#include<ccan/json_out/json_out.h>
#include<ccan/take/take.h>
#include<ccan/tal/str/str.h>
#include<common/json.h>
#include<common/json_stream.h>
#include<common/utils.h>
#include<plugins/payz/json_equal.h>
#include<plugins/payz/tester/command.h>
#include<plugins/payz/tester/loop.h>
#include<plugins/payz/tester/rpc.h>
#include<plugins/payz/tester/spawn.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

/*-----------------------------------------------------------------------------
Constants
-----------------------------------------------------------------------------*/

#define TESTER_TIMEOUT (time_from_sec(60))

/*-----------------------------------------------------------------------------
Top Object
-----------------------------------------------------------------------------*/

struct payz_tester {
	/* Sub-objects.  */
	struct payz_tester_spawn *spawn;
	struct payz_tester_rpc *rpc;
	struct payz_tester_command *command;

	/* Command counter ID.  */
	u64 id;
	/* Spare buffers.  */
	char *buffer;
	jsmntok_t *toks;
};

static bool wait_for_response(struct payz_tester *tester);

static struct payz_tester *
payz_tester_new(const tal_t *ctx,
		struct payz_tester_spawn **spawn)
{
	struct timemono start = time_mono();

	struct payz_tester *tester;

	struct json_stream *js;
	const char *init_params;
	size_t init_params_len;

	tester = tal(ctx, struct payz_tester);
	tester->rpc = payz_tester_rpc_new(tester, TESTER_TIMEOUT);
	tester->command = payz_tester_command_new(tester,
						  TESTER_TIMEOUT,
						  payz_tester_spawn_stdin(*spawn),
						  payz_tester_spawn_stdout(*spawn));
	tester->id = 1;
	tester->buffer = NULL;
	tester->toks = NULL;
	/* Grab control of the spawn.  */
	tester->spawn = tal_steal(tester, *spawn);
	*spawn = NULL;

	/* Manifest the plugin.  */
	payz_tester_command_send(tester->command, start, tester->id,
				 "getmanifest",
				 "{\"allow-deprecated-apis\": true}");
	payz_tester_loop("getmanifest",
			 tester->command, tester->rpc, tester->spawn,
			 TESTER_TIMEOUT,
			 &wait_for_response, tester);
	tester->buffer = tal_free(tester->buffer);
	tester->toks = tal_free(tester->toks);

	/* Init the plugin, this requires a more complicated params object.
	 */
	js = new_json_stream(tmpctx, NULL, NULL);
	json_object_start(js, NULL);
	/* Configuration.  */
	json_object_start(js, "configuration");
	json_add_string(js, "lightning-dir", payz_tester_rpc_dir(tester->rpc));
	json_add_string(js, "rpc-file", "lightning-rpc");
	json_add_string(js, "network", "testnet");
	json_object_start(js, "feature_set");
	json_object_end(js);
	json_object_end(js);
	/* Dummy options object.  */
	json_object_start(js, "options");
	json_object_end(js);
	json_object_end(js);
	json_out_finished(js->jout);
	/* Send the init command.  */
	init_params = json_out_contents(js->jout, &init_params_len);
	payz_tester_command_send(tester->command, start, tester->id,
				 "init",
				 tal_strndup(tmpctx,
					     init_params, init_params_len));
	/* Wait for response.
	 * Warning: payz_tester_loop will clean tmpctx, so the js is already
	 * freed after this point.
	 */
	payz_tester_loop("init",
			 tester->command, tester->rpc, tester->spawn,
			 TESTER_TIMEOUT,
			 &wait_for_response, tester);
	tester->buffer = tal_free(tester->buffer);
	tester->toks = tal_free(tester->toks);

	return tester;
}

static bool
payz_tester_command_impl(struct payz_tester *tester,
			 const char **buffer,
			 const jsmntok_t **toks,
			 const char *method,
			 const char *params)
{
	struct timemono start = time_mono();

	const jsmntok_t *result;
	const jsmntok_t *error;

	char *source = tal_fmt(NULL, "payz_tester_command(%s, %s)", method, params);

	tester->buffer = tal_free(tester->buffer);
	tester->toks = tal_free(tester->toks);

	payz_tester_command_send(tester->command, start, tester->id,
				 method, params);
	payz_tester_loop(source,
			 tester->command, tester->rpc, tester->spawn,
			 TESTER_TIMEOUT,
			 &wait_for_response, tester);

	*buffer = tester->buffer;

	result = json_get_member(tester->buffer, tester->toks,
				 "result");
	error = json_get_member(tester->buffer, tester->toks,
				"error");
	if (result) {
		tal_free(source);
		*toks = result;
		return true;
	}
	if (error) {
		tal_free(source);
		*toks = error;
		return false;
	}

	errx(1,
	    "%s: Response from plugin did not have "
	    "result or error: %.*s",
	    source,
	    json_tok_full_len(tester->toks),
	    json_tok_full(tester->buffer, tester->toks));
}

static bool wait_for_response(struct payz_tester *tester)
{
	const char *error;
	u64 id;

	if (!payz_tester_command_get_response(tester,
					      tester->command,
					      &tester->buffer,
					      &tester->toks))
		/* Keep going.  */
		return true;

	/* Check response, it should match the id.  */
	error = json_scan(tmpctx, tester->buffer, tester->toks,
			  "{id:%}",
			  JSON_SCAN(json_to_u64, &id));
	if (!error && id != tester->id)
		error = tal_fmt(tmpctx,
				"incorrrect 'id': expected %"PRIu64", "
				"got %"PRIu64,
				tester->id, id);
	if (error)
		errx(1, "Plugin handshake: Response: %s", error);
	/* *Now* increment the id.  */
	tester->id++;
	/* We can finish now.  */
	return false;
}

/*-----------------------------------------------------------------------------
Static Top Object
-----------------------------------------------------------------------------*/

static struct payz_tester *payz_tester = NULL;
static struct payz_tester_spawn *spawn = NULL;

static void payz_tester_atexit(void);

void payz_tester_init(const char *argv0)
{
	int res;

	/* Call this before anything else, to reduce any garbage in
	 * the spawned process.
	 */
	spawn = payz_tester_spawn_new(TESTER_TIMEOUT);
	if (!spawn) {
		err_set_progname(argv0);
		errx(1, "Unable to spawn sub-provess for plugin.");
	}

	/* Set up exit handler.  */
	res = atexit(&payz_tester_atexit);
	if (res != 0) {
		spawn = tal_free(spawn);
		err_set_progname(argv0);
		errx(1, "Unable to set atexit handler.");
	}
	/* Set up various necessary things.  */
	setup_locale();
	err_set_progname(argv0);
	setup_tmpctx();
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	/* Hand over control of spawn.  */
	payz_tester = payz_tester_new(NULL, &spawn);
}

/* Exit.  */
static void payz_tester_atexit(void)
{
	const char *p = taken_any();
	if (p)
		errx(1, "outstanding taken(): %s", p);
	take_cleanup();
	tmpctx = tal_free(tmpctx);
	spawn = tal_free(spawn);
	payz_tester = tal_free(payz_tester);
}

/*-----------------------------------------------------------------------------
Commands to the plugin
-----------------------------------------------------------------------------*/

bool payz_tester_command(const char **buffer,
			 const jsmntok_t **toks,
			 const char *method,
			 const char *params)
{
	return payz_tester_command_impl(payz_tester,
					buffer, toks,
					method, params);
}

void payz_tester_command_expect(const char *method,
				const char *params,
				const char *expected)
{
	bool ret;
	const char *buffer;
	const jsmntok_t *result;
	jsmntok_t *exptoks;

	ret = payz_tester_command(&buffer, &result, method, params);
	if (!ret)
		errx(1, "payz_tester_command_expect(%s, %s, %s): "
		     "Command failed with: %.*s",
		     method, params, expected,
		     json_tok_full_len(result),
		     json_tok_full(buffer, result));

	/* Parse expected.  */
	exptoks = json_parse_simple(tmpctx, expected, strlen(expected));

	if (!json_equal(buffer, result, expected, exptoks))
		errx(1, "payz_tester_command_expect(%s, %s, %s): "
		     "Unexpected 'result': %.*s",
		     method, params, expected,
		     json_tok_full_len(result),
		     json_tok_full(buffer, result));
}

void payz_tester_command_expectfail(const char *method,
				    const char *params,
				    errcode_t expected_code)
{
	bool ret;
	const char *buffer;
	const jsmntok_t *result;

	const char *error;
	errcode_t actual_code;

	ret = payz_tester_command(&buffer, &result, method, params);
	if (ret)
		errx(1, "payz_tester_command_expectfail(%s, %s, "
		     "%"PRIerrcode"): Command succeeded? %.*s",
		     method, params, expected_code,
		     json_tok_full_len(result),
		     json_tok_full(buffer, result));

	error = json_scan(tmpctx, buffer, result,
			  "{code:%}",
			  JSON_SCAN(json_to_errcode, &actual_code));
	if (error)
		errx(1, "payz_tester_command_expectfail(%s, %s, "
		     "%"PRIerrcode"): Failed to parse error: %s",
		     method, params, expected_code,
		     error);

	if (actual_code != expected_code)
		errx(1, "payz_tester_command_expectfail(%s, %s, "
		     "%"PRIerrcode"): Expected error code %"PRIerrcode", "
		     "got %"PRIerrcode"",
		     method, params, expected_code,
		     expected_code, actual_code);
}

void payz_tester_command_ok(const char *method,
			    const char *params)
{
	bool ret;
	const char *buffer;
	const jsmntok_t *result;

	ret = payz_tester_command(&buffer, &result, method, params);
	if (!ret)
		errx(1, "payz_tester_command_ok(%s, %s): "
		     "Command failed! %.*s",
		     method, params,
		     json_tok_full_len(result),
		     json_tok_full(buffer, result));
}

/*-----------------------------------------------------------------------------
Component Waiting
-----------------------------------------------------------------------------*/

static void
payz_tester_wait_component_generic(const char *api_name,
				   bool expected_found,
				   const char **buffer,
				   const jsmntok_t **component,
				   u32 entity,
				   const char *component_name)
{
	struct timemono start = time_mono();

	const jsmntok_t *result;

	bool found = false;
	bool ret;

	do {
		const char *params;

		if (time_greater(timemono_since(start), TESTER_TIMEOUT)) {
			const char *rbuf;
			const jsmntok_t *result;

			char *systrace;
			char *listentities;

			(void) payz_tester_command(&rbuf, &result,
						   "payecs_systrace",
						   tal_fmt(tmpctx,
							   "[%"PRIu32"]",
							   entity));
			systrace = tal_strndup(NULL,
					       json_tok_full(rbuf, result),
					       json_tok_full_len(result));
			(void) payz_tester_command(&rbuf, &result,
						   "payecs_listentities",
						   "[]");
			listentities = tal_strndup(systrace,
						   json_tok_full(rbuf, result),
						   json_tok_full_len(result));
			tal_steal(tmpctx, systrace);
			errx(1,
			    "%s(%"PRIu32", %s): "
			    "Timed out!\n"
			    "\nsystrace: %s\n"
			    "\nlistentities: %s\n",
			    api_name,
			    entity, component_name,
			    systrace, listentities);
		}

		/* Since we use tmpctx and payz_tester_command clears that,
		 * we have to regen the params at each loop iteration.
		 */
		params = tal_fmt(tmpctx, "[%"PRIu32", [\"%s\"]]",
				 entity, component_name);
		ret = payz_tester_command(buffer, &result,
					  "payecs_getcomponents", params);
		if (!ret)
			errx(1,
			     "%s(%"PRIu32", %s): "
			     "payecs_getcomponents failed! %.*s",
			     api_name,
			     entity, component_name,
			     json_tok_full_len(result),
			     json_tok_full(*buffer, result));

		*component = json_get_member(*buffer, result, component_name);
		if (!(*component))
			errx(1,
			     "%s(%"PRIu32", %s): "
			     "payecs_getcomponents did not return "
			     "component??? %.*s",
			     api_name,
			     entity, component_name,
			     json_tok_full_len(result),
			     json_tok_full(*buffer, result));
		found = !json_tok_is_null(*buffer, *component);
	} while (found != expected_found);
}

void payz_tester_wait_component(const char **buffer,
				const jsmntok_t **component,
				u32 entity,
				const char *component_name)
{
	/* Wait for "found" to be true.  */
	payz_tester_wait_component_generic("payz_tester_wait_component",
					   true,
					   buffer, component,
					   entity, component_name);
}
void payz_tester_wait_detach_component(u32 entity,
				       const char *component_name)
{
	/* Dummy variables.  */
	const char *buffer;
	const jsmntok_t *component;

	/* Wait for "found" to be false.  */
	payz_tester_wait_component_generic("payz_tester_wait_detach_component",
					   false,
					   &buffer, &component,
					   entity, component_name);
}

