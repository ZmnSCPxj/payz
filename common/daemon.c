#include"daemon.h"
#include<ccan/compiler/compiler.h>
#include<ccan/err/err.h>
#include<ccan/io/io.h>
#include<ccan/str/str.h>
#include<ccan/take/take.h>
#include<ccan/tal/str/str.h>
#include<common/utils.h>
#include<signal.h>
#include<stdio.h>
#include<poll.h>

/* NOTE!
 *
 * Even though the corresponding header file is
 * verbatim from C-Lightning, this source file is
 * not.
 *
 * Mostly we remove the libwally and libsecp256k1
 * initializations that are in common_setup, and
 * merge that into daemon_setup.
 */

void daemon_setup(const char *argv0,
		  void (*backtrace_print)(const char *fmt, ...) UNUSED,
		  void (*backtrace_exit)(void) UNUSED)
{
	setup_locale();
	err_set_progname(argv0);
	setup_tmpctx();

	/* We handle write returning errors! */
	signal(SIGPIPE, SIG_IGN);

	io_poll_override(daemon_poll);
}

void send_backtrace(const char *why UNUSED)
{
}

int daemon_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	const char *t;

	t = taken_any();
	if (t)
		errx(1, "Outstanding taken pointers: %s", t);

	clean_tmpctx();

	return poll(fds, nfds, timeout);
}

void daemon_shutdown(void)
{
	const char *p = taken_any();
	if (p)
		errx(1, "outstanding taken(): %s", p);
	take_cleanup();
	tmpctx = tal_free(tmpctx);
}

void daemon_maybe_debug(char *argv[])
{
	for (int i = 1; argv[i]; i++) {
		if (!streq(argv[i], "--debugger"))
			continue;

		/* Don't let this mess up stdout, so redir to /dev/null */
		char *cmd = tal_fmt(NULL, "${DEBUG_TERM:-gnome-terminal --} gdb -ex 'attach %u' %s >/dev/null &", getpid(), argv[0]);
		fprintf(stderr, "Running %s\n", cmd);
		/* warn_unused_result is fascist bullshit.
		 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425 */
		if (system(cmd))
			;
		/* Continue in the debugger. */
		kill(getpid(), SIGSTOP);
	}
}
