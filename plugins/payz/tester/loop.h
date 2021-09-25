#ifndef LIGHTNING_PLUGINS_PAYZ_TESTER_LOOP_H
#define LIGHTNING_PLUGINS_PAYZ_TESTER_LOOP_H
#include"config.h"
#include<ccan/take/take.h>
#include<ccan/time/time.h>
#include<ccan/typesafe_cb/typesafe_cb.h>

struct payz_tester_command;
struct payz_tester_rpc;
struct payz_tester_spawn;

/** payz_tester_loop
 *
 * @brief Perform event loop until the given callback returns
 * false.
 * The callback is called at each loop iteration.
 *
 * @desc This will clear tmpctx, so none of the arguments can
 * safely be owned by tmpctx.
 *
 * @param source - string description of the caller, prepended
 * to error messages, can be take().
 * @param command - the command handler object.
 * @param rpc - the RPC emulator handler object.
 * @param spawn - the plugin process handler object.
 * @param timeout - How long to loop before erroring out.
 * @param cb - the per-iteration callback; return true to keep
 * iteating, false to stop.
 * @param cbarg - the callback argument.
 */
void payz_tester_loop_(const char *source,
		       struct payz_tester_command *command,
		       struct payz_tester_rpc *rpc,
		       struct payz_tester_spawn *spawn,
		       struct timerel timeout,
		       bool (*cb)(void *cbarg),
		       void *cbarg);
#define payz_tester_loop(src, c, r, s, to, cb, arg) \
	payz_tester_loop_((src), (c), (r), (s), (to), \
			  typesafe_cb(bool, void *, (cb), (arg)), \
			  (arg))

#endif /* LIGHTNING_PLUGINS_PAYZ_TESTER_LOOP_H */
