#ifndef LIGHTNING_PLUGINS_PAYZ_TESTER_RPC_H
#define LIGHTNING_PLUGINS_PAYZ_TESTER_RPC_H
#include"config.h"
#include<ccan/take/take.h>
#include<ccan/tal/tal.h>
#include<ccan/time/time.h>
#include<common/errcode.h>

/** struct payz_tester_rpc
 *
 * @brief Object that handles the testing framework RPC emulation.
 */
struct payz_tester_rpc;

/** payz_tester_rpc_new
 *
 * @brief Construct the RPC emulator.
 *
 * @param ctx - the owner of the RPC emulator.
 * @param timeout - the maximum amount of time that writes to
 * the plugin can take before the RPC emulator aborts.
 *
 * @return - the constructed RPC emulator.
 */
struct payz_tester_rpc *payz_tester_rpc_new(const tal_t *ctx,
					    struct timerel timeout);

/** payz_tester_rpc_dir
 *
 * @brief Get the directory used for RPC emulation.
 *
 * @desc This should get fed as the `lightning-dir` field of
 * the `configuration` passed in the `init` method to the
 * plugin.
 * You should set `rpc-file` to `lightning-rpc`.
 *
 * @param rpc - the RPC emulator object.
 *
 * @return - the directory.
 * The storage for this string is owned by the RPC emulator
 * object and remains valid for the lifetime of the RPC
 * emulator.
 */
const char *payz_tester_rpc_dir(const struct payz_tester_rpc *rpc);

/** payz_tester_rpc_fds
 *
 * @brief Get the file descriptors used on the socket for the
 * RPC emulation.
 *
 * @desc The caller should use this in a poll() or select() or
 * similar event system.
 * All FDs should be checked for input availability.
 *
 * @param rpc - the RPC emulator object.
 *
 * @return - a tal-allocated array of file descriptors, use
 * tal_count to determine number of fds.
 * Storage is owned by the RPC emulator object and remains
 * valid for its lifetime, or when you next call
 * payz_tester_rpc_check.
 */
const int *payz_tester_rpc_fds(const struct payz_tester_rpc *rpc);

/** payz_tester_rpc_check
 *
 * @brief Inform the RPC emulator that one of the fds has
 * an event.
 *
 * @desc This call does not block on input, but may block
 * on output.
 * If it blocks on output, if the timeout is reached, it
 * will abort the program with an error.
 *
 * @param rpc - the RPC emulator object.
 */
void payz_tester_rpc_check(struct payz_tester_rpc *rpc);

/** payz_tester_rpc_has_incoming
 *
 * @brief Check if there are pending RPC requests from
 * the plugin.
 *
 * @param rpc - the RPC emulator object.
 *
 * @return - true if there are complete RPC requests
 * that we have not responded to since there is no
 * queued up response for them.
 */
bool payz_tester_rpc_has_incoming(const struct payz_tester_rpc *rpc);

/** payz_tester_rpc_has_outgoing
 *
 * @brief Check if there are pending RPC responses to
 * the plugin which the plugin has not requested yet.
 *
 * @param rpc - the RPC emulator object.
 *
 * @return - true if there are queued up responses
 * to RPC commands that hae not been requested by
 * the plugin.
 */
bool payz_tester_rpc_has_outgoing(const struct payz_tester_rpc *rpc);

/** payz_tester_rpc_clear
 *
 * @brief Remove any pending RPC responses to the
 * plugin.
 *
 * @param rpc - the RPC emulator object.
 */
void payz_tester_rpc_clear(struct payz_tester_rpc *rpc);

/** payz_tester_rpc_add_response
 *
 * @brief Provide a response.
 * 
 * @desc This is given a string containing the JSON value to
 * send.
 * This may block on writing a response to the plugin, but
 * only up to the given timeout.
 *
 * This will enqueue an RPC success response.
 *
 * @param rpc - the RPC emulator object.
 * @param method - the RPC method to respond to, can be take().
 * @param data - the response to the RPC, a nul-terminated C
 * string containing valid JSON data, can be take().
 */
void payz_tester_rpc_add_response(struct payz_tester_rpc *rpc,
				  const char *method TAKES,
				  const char *data TAKES);

/** payz_tester_rpc_add_error
 *
 * @brief Provide an error return to a particular RPC.
 *
 * @desc Like above, given a string containing the JSON value
 * to send.
 * May block, as above.
 *
 * This will enqueue an RPC error response.
 * A single queue is used for RPC error and success responses.
 *
 * @param rpc - the RPC emulator object.
 * @param method - the RPC method to respond to, can be take().
 * @param code - the error code to return.
 * @param message - the error message to return, a JSON-formatted
 * string, can be take()
 * @param data - an optional error data object to return, a
 * nul-termianted C string containing valid JSON data, can be
 * take().
 */
void payz_tester_rpc_add_error(struct payz_tester_rpc *rpc,
			       const char *method TAKES,
			       errcode_t code,
			       const char *message TAKES,
			       const char *data TAKES);

#endif /* LIGHTNING_PLUGINS_PAYZ_TESTER_RPC_H */
