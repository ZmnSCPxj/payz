#ifndef LIGHTNING_PLUGINS_PAYZ_TESTER_COMMAND_H
#define LIGHTNING_PLUGINS_PAYZ_TESTER_COMMAND_H
#include"config.h"
#include<ccan/short_types/short_types.h>
#include<ccan/tal/tal.h>
#include<ccan/time/time.h>
#include<common/json.h>

/** struct payz_tester_command
 *
 * @brief Object that handles the command interface to
 * the plugin under test.
 */
struct payz_tester_command;

/** payz_tester_command_new
 *
 * @brief Constructs a command handler.
 *
 * @desc Neither fd is closed by this object; the spawn
 * object handles that.
 *
 * @param ctx - owner of the command handler.
 * @param timeout - the maximum timeout for writes to the
 * plugin.
 * @param to_stdin - fd of the stdin of the plugin.
 * @param from_stdout - fd of the stdout of the plugin.
 *
 * @return - the newly constructed command handler.
 */
struct payz_tester_command *
payz_tester_command_new(const tal_t *ctx,
			struct timerel timeout,
			int to_stdin,
			int from_stdout);

/** payz_tester_command_check
 *
 * @brief Inform the command handler that the from_stdout
 * fd has an event.
 *
 * @desc Log outputs will be printed immediately on our own
 * stdout.
 * Responses and non-log notifications are put in queues.
 *
 * @param command - the command handler.
 */
void payz_tester_command_check(struct payz_tester_command *command);

/** payz_tester_command_get_response
 *
 * @brief Gets a queued command response if available.
 *
 * @param ctx - the owner of the returned response.
 * @param command - the command handler.
 * @param buffer - output, the string buffer containing JSON text.
 * @param tok - output, the token array for the parsed JSON.
 *
 * @return - true if a response was dequeued, false if no
 * responses.
 */
bool
payz_tester_command_get_response(const tal_t *ctx,
				 struct payz_tester_command *command,
				 char **buffer,
				 jsmntok_t **tok);

/** payz_tester_command_get_notif
 *
 * @brief Gets a queued notification if available.
 *
 * @param ctx - the owner of the returned response.
 * @param command - the command handler.
 * @param buffer - output, the string buffer containing JSON text.
 * @param tok - output, the token array for the parsed JSON.
 *
 * @return - true if a response was dequeued, false if no
 * responses.
 */
bool
payz_tester_command_get_notif(const tal_t *ctx,
			      struct payz_tester_command *command,
			      char **buffer,
			      jsmntok_t **tok);

/** payz_tester_command_send
 *
 * @brief Sends a command to the plugin.
 *
 * @param command - the command handler.
 * @param start - time to use as reference for timeouts.
 * @param id - ID number to use.
 * @param method - the method name, nul-terminated C string without
 * any escape-requiring chars.
 * @param params - the parameters to the method, a nul-terminated C
 * string containing properly formatted JSON text.
 */
void payz_tester_command_send(struct payz_tester_command *command,
			      struct timemono start,
			      u64 id,
			      const char *method,
			      const char *params);

#endif /* LIGHTNING_PLUGINS_PAYZ_TESTER_COMMAND_H */
