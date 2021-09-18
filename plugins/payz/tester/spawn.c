#include"spawn.h"
#include<ccan/err/err.h>
#include<ccan/tal/tal.h>
#include<errno.h>
#include<plugins/payz/main.h>
#include<poll.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

struct payz_tester_spawn {
	/* Child PID.  */
	pid_t child;

	/* Pipes to child.  */
	int to_stdin;
	int from_stdout;

	/* Timeout.  */
	struct timerel timeout;
};

static int payz_tester_spawn_child(int child_stdin, int child_stdout);
static void payz_tester_spawn_destructor(struct payz_tester_spawn *spawn);

struct payz_tester_spawn *payz_tester_spawn_new(struct timerel timeout)
{
	struct payz_tester_spawn *spawn;

	/* [0] is the read end, [1] is the write end.  */
	int pipe_stdin[2];
	int pipe_stdout[2];

	int pipe_res;

	int status;

	pid_t pid;

	/* Construct the pipes, then fork, *before* constructing any
	 * objects from tal.  */
	pipe_res = pipe(pipe_stdin);
	if (pipe_res < 0)
		err(1, "spawn: pipe(stdin)");
	pipe_res = pipe(pipe_stdout);
	if (pipe_res < 0)
		err(1, "spawn: pipe(stdin)");

	pid = fork();
	if (pid < 0)
		err(1, "spawn: fork");

	if (pid == 0) {
		/* Close parent ends of the pipes.  */
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);

		status = payz_tester_spawn_child(pipe_stdin[0],
						 pipe_stdout[1]);

		exit(status);
	}

	/* *Now* construct the object.  */
	spawn = tal(NULL, struct payz_tester_spawn);
	spawn->child = pid;
	spawn->to_stdin = pipe_stdin[1];
	spawn->from_stdout = pipe_stdout[0];
	spawn->timeout = timeout;
	tal_add_destructor(spawn, &payz_tester_spawn_destructor);

	return spawn;
}

static int payz_tester_spawn_child(int child_stdin, int child_stdout)
{
	int dup2_res;

	char argv0[5];
	char *argv[1];

	/* Redirect stdin and stdout.  */
	do {
		dup2_res = dup2(child_stdin, STDIN_FILENO);
	} while (dup2_res < 0 && errno == EINTR);
	if (dup2_res < 0)
		err(1, "child: stdin: dup2(%d, %d)", child_stdin, STDIN_FILENO);
	do {
		dup2_res = dup2(child_stdout, STDOUT_FILENO);
	} while (dup2_res < 0 && errno == EINTR);
	if (dup2_res < 0)
		err(1, "child: stdout: dup2(%d, %d)", child_stdout, STDOUT_FILENO);

	/* Close the now-extraneous descriptors.  */
	close(child_stdin);
	close(child_stdout);

	/* Now call the plugin code.  */
	strcpy(argv0, "payz");
	argv[0] = argv0;
	return payz_main(1, argv, "payz", "keysendz");
}

static void payz_tester_spawn_destructor(struct payz_tester_spawn *spawn)
{
	struct timemono start = time_mono();
	struct timerel time;

	pid_t pid;
	int status;

	int poll_res;
	struct pollfd pollfd;
	char buffer[4096];
	ssize_t nread;

	/* First, close the stdin of the plugin, which should
	 * cause the plugin to terminate due to EOF.
	 */
	close(spawn->to_stdin);

	/* Wait for plugin to die.  */
	for (;;) {
		/* Timed out?  */
		if (time_greater(timemono_since(start), spawn->timeout))
			errx(1, "Child did not exit within timeout.");

		pid = waitpid(spawn->child, &status, WNOHANG);
		if (pid < 0)
			err(1, "waitpid(%d)", (int) spawn->child);
		if (pid != 0) {
			/* Did the child close with an error?  */
			if (status != 0)
				errx(1,
				     "Child exited with non-zero status: %d",
				     status);
			break;
		}

		/* Sleep a short while to prevent 100% CPU.  */
		time = time_from_msec(20);
		nanosleep(&time.ts, NULL);
	}

	/* Now slurp data from stdout.
	 * Due to the plugin closing, it should also get closed and
	 * trigger an EOF on our side as well.
	 */
	for (;;) {
		/* Timed out?  */
		if (time_greater(timemono_since(start), spawn->timeout))
			errx(1, "Child did not close stdout within timeout.");

		/* Poll for 20 milliseconds so we can check for timeout.
		 *
		 * We could be more sophisticated here and measure the time
		 * since we started and when our own timeout is and pass
		 * that in to ppoll instead, but the current version works
		 * reasonably enough.
		 */
		pollfd.fd = spawn->from_stdout;
		pollfd.events = POLLIN;
		poll_res = poll(&pollfd, 1, 20);
		if (poll_res < 0 && errno != EINTR)
			err(1, "poll(%d)", spawn->from_stdout);
		if (poll_res > 0) {
			nread = read(spawn->from_stdout, buffer, sizeof(buffer));
			/* EOF?  */
			if (nread == 0)
				break;
		}
	}
	/* Finally clsoe the stdout in-pipe.  */
	close(spawn->from_stdout);
}

int payz_tester_spawn_stdin(const struct payz_tester_spawn *spawn)
{
	return spawn->to_stdin;
}

int payz_tester_spawn_stdout(const struct payz_tester_spawn *spawn)
{
	return spawn->from_stdout;
}
