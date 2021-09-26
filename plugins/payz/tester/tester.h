#ifndef LIGHTNING_PLUGINS_PAYZ_TESTER_TESTER_H
#define LIGHTNING_PLUGINS_PAYZ_TESTER_TESTER_H
#include"config.h"
#include<common/json.h>
#include<stdbool.h>

/** payz_tester_init
 *
 * @brief Call at the start of a test program to start the
 * plugin in a separate process and set up testing.
 *
 * @desc This installs an atexit() handler to perform
 * cleanup.
 *
 * @param argv9 - the name of the test program, from
 * argv[0].
 */
void payz_tester_init(const char *argv0);

/** payz_tester_command
 *
 * @brief Send a command and parameters, and acquire
 * the result or error.
 *
 * @desc This is the most general and least easy-to-use.
 *
 * This function will cause tmpctx to be cleaned!
 * Also, the returned buffer and array will be reused.
 *
 * @param buffer - output, a buffer containing JSON text.
 * The buffer is invalidated by the next call to
 * payz_tester_command.
 * @param toks - output, an array of tokens, the [0]th
 * being either the result, or the error object.
 * It is possibly in the middle of a tal-allocated array
 * and not at the start, meaning you cannot use tal_count
 * on it safely.
 * The array is invalidated by the next call to
 * payz_tester_command.
 * @param method - the method to invoke.
 * @param params - the parameters to provide.
 *
 * @return - true if the command succeeded, false if the
 * ommand failed.
 */
bool payz_tester_command(const char **buffer,
			 const jsmntok_t **toks,
			 const char *method,
			 const char *params);

/** payz_tester_command_expect
 *
 * @brief Send a command and parameters, and assert that
 * the specific response is returned.
 *
 * @desc This function calls into payz_tester_command.
 *
 * @param method - the method to call in the plugin.
 *
 */
void payz_tester_command_expect(const char *method,
				const char *params,
				const char *expected);

#endif /* LIGHTNING_PLUGINS_PAYZ_TESTER_TESTER_H */
