#include"loop.h"
#include<ccan/err/err.h>
#include<ccan/tal/tal.h>
#include<common/utils.h>
#include<errno.h>
#include<plugins/payz/tester/command.h>
#include<plugins/payz/tester/rpc.h>
#include<plugins/payz/tester/spawn.h>
#include<poll.h>
#include<unistd.h>

void payz_tester_loop_(const char *source,
		       struct payz_tester_command *command,
		       struct payz_tester_rpc *rpc,
		       struct payz_tester_spawn *spawn,
		       struct timerel timeout,
		       bool (*cb)(void *cbarg),
		       void *cbarg)
{
	struct timemono start = time_mono();

	size_t i;
	const int *rpcfds;
	struct pollfd *pfds;
	int poll_res;

	for (;;) {
		clean_tmpctx();

		if (!cb(cbarg))
			break;

		if (time_greater(timemono_since(start), timeout))
			errx(1, "%s: Timed out while waiting for plugin.",
			     source);

		rpcfds = payz_tester_rpc_fds(rpc);

		/* Build the array for poll().  */
		pfds = tal_arr(tmpctx, struct pollfd, 1);
		pfds[0].fd = payz_tester_spawn_stdout(spawn);
		pfds[0].events = POLLIN;
		for (i = 0; i < tal_count(rpcfds); ++i) {
			struct pollfd pfd1;
			pfd1.fd = rpcfds[i];
			pfd1.events = POLLIN;
			tal_arr_expand(&pfds, pfd1);
		}
		/* Now perform a poll.  */
		poll_res = poll(pfds, tal_count(pfds), 20);
		if (poll_res < 0 && errno == EINTR)
			continue;
		if (poll_res < 0)
			err(1, "%s: poll()", source);
		if (poll_res == 0)
			continue;

		/* Check for events.  */
		for (i = 0; i < tal_count(pfds); ++i) {
			if (pfds[i].revents == 0)
				continue;
			if (i == 0)
				payz_tester_command_check(command);
			else {
				payz_tester_rpc_check(rpc);
				/* The RPC emulator will check all its
				 * fds, so we can break out of this
				 * loop now.
				 */
				break;
			}
		}
	}

	clean_tmpctx();
}
