#include"command.h"
#include<ccan/err/err.h>
#include<ccan/list/list.h>
#include<ccan/tal/str/str.h>
#include<common/utils.h>
#include<errno.h>
#include<plugins/payz/ecs/ecs.h>
#include<poll.h>
#include<stdio.h>
#include<unistd.h>

struct payz_tester_command {
	/* Construction arguments.  */
	struct timerel timeout;
	int to_stdin;
	int from_stdout;

	/* Parser state.  */
	jsmn_parser parser;
	char *buffer;
	jsmntok_t *toks;

	/* Queues for the responses and notifications.  */
	struct list_head responses;
	struct list_head notifs;
};

/* Queue entry.  */
struct payz_tester_command_qentry {
	struct list_node list;
	char *buffer;
	jsmntok_t *toks;
};

static void write_timed(int fd, const void *vbuffer, size_t len,
			struct timemono start, struct timerel timeout);

/*-----------------------------------------------------------------------------
Construction
-----------------------------------------------------------------------------*/

struct payz_tester_command *
payz_tester_command_new(const tal_t *ctx,
			struct timerel timeout,
			int to_stdin,
			int from_stdout)
{
	struct payz_tester_command *command;

	command = tal(ctx, struct payz_tester_command);
	command->timeout = timeout;
	command->to_stdin = to_stdin;
	command->from_stdout = from_stdout;
	jsmn_init(&command->parser);
	command->buffer = tal_arr(command, char, 0);
	command->toks = toks_alloc(command);
	list_head_init(&command->responses);
	list_head_init(&command->notifs);

	return command;
}

/*-----------------------------------------------------------------------------
Receiving
-----------------------------------------------------------------------------*/

/* Process a single object from the plugin.  */
static void
payz_tester_command_process(struct payz_tester_command *command,
			    const char *buffer,
			    const jsmntok_t *toks);

void payz_tester_command_check(struct payz_tester_command *command)
{
	char tmpbuf[4096];
	ssize_t nread;

	bool complete;

	do {
		nread = read(command->from_stdout, tmpbuf, sizeof(tmpbuf));
	} while (nread < 0 && errno == EINTR);
	if (nread < 0)
		err(1, "payz_tester_command: read(%d)", command->from_stdout);

	/* Unexpected EOF!
	 * We expect the plugin to keep stdout open until it gets
	 * terminated by receiving an EOF from its stdin.
	 */
	if (nread == 0)
		errx(1, "payz_tester_command: Unexpected EOF from plugin.");

	/* Append to buffer.  */
	tal_expand(&command->buffer, tmpbuf, nread);

	do {
		if (!json_parse_input(&command->parser, &command->toks,
				      command->buffer,
				      tal_count(command->buffer),
				      &complete))
			errx(1,
			     "plugin stdout: malformed JSON text in "
			     "response: %.*s",
			     (int) tal_count(command->buffer), command->buffer);

		if (!complete)
			return;

		/* json_parse_input returns a single-token array if
		 * only whitespace left.  */
		if (tal_count(command->toks) == 1) {
			tal_resize(&command->buffer, 0);
			jsmn_init(&command->parser);
			toks_reset(command->toks);
			return;
		}

		/* Process a single response.  */
		payz_tester_command_process(command,
					    command->buffer, command->toks);

		/* Consume one response from the buffer and
		 * reset the parser.
		 */
		memmove(command->buffer,
			command->buffer + command->toks[0].end,
			tal_count(command->buffer) - command->toks[0].end);
		tal_resize(&command->buffer,
			   tal_count(command->buffer) - command->toks[0].end);
		jsmn_init(&command->parser);
		toks_reset(command->toks);

	} while (tal_count(command->buffer) != 0);
}

static void
payz_tester_command_process(struct payz_tester_command *command,
			    const char *buffer,
			    const jsmntok_t *toks)
{
	struct payz_tester_command_qentry *qe;
	bool is_notif;
	const jsmntok_t *method;
	const jsmntok_t *params;

	is_notif = !json_get_member(buffer, toks, "id");

	method = json_get_member(buffer, toks, "method");
	if (is_notif && !method)
		errx(1, "plugin stdout: No 'method' in response: %.*s",
		     json_tok_full_len(toks), json_tok_full(buffer, toks));

	params = json_get_member(buffer, toks, "params");
	if (is_notif && !params)
		errx(1,
		     "plugin stdout: No 'params' in "
		     "notification: %.*s",
		     json_tok_full_len(toks),
		     json_tok_full(buffer, toks));

	/* Print out log entries directly to our own stdout.  */
	if (is_notif && json_tok_streq(buffer, method, "log")) {
		const char *error;
		char *level;
		char *message;

		error = json_scan(tmpctx, buffer, params,
				  "{level:%,message:%}",
				  JSON_SCAN_TAL(tmpctx, json_strdup, &level),
				  JSON_SCAN_TAL(tmpctx, json_strdup, &message));
		if (error)
			errx(1, "plugin stdout: 'params': %s", error);

		printf("%s: %s\n", level, message);

		return;
	}
	/* Reflect payecs_system_trigger notifications back to the
	 * plugin.
	 */
	if (is_notif && json_tok_streq(buffer, method,
				       ECS_SYSTEM_NOTIFICATION)) {
		/* lightningd adds an additional `payload` wrapper
		 * and an `origin` field to the `params`.
		 */
		const char *notif;

		notif = tal_fmt(tmpctx,
				"{\"jsonrpc\": \"2.0\","
				" \"method\": \""ECS_SYSTEM_NOTIFICATION"\","
				" \"params\": "
				"   {\"origin\": \"payz\","
				"    \"payload\": %.*s}}",
				json_tok_full_len(params),
				json_tok_full(buffer, params));

		write_timed(command->to_stdin,
			    notif, strlen(notif),
			    time_mono(), command->timeout);
		/* Do not return, let it be added to notifications
		 * list.
		 */
	}

	qe = tal(command, struct payz_tester_command_qentry);
	qe->buffer = tal_strndup(qe, buffer, toks[0].end);
	qe->toks = tal_dup_arr(qe, jsmntok_t,
			       toks, json_next(&toks[0]) - &toks[0],
			       0);

	if (is_notif)
		list_add_tail(&command->notifs, &qe->list);
	else
		list_add_tail(&command->responses, &qe->list);
}

/*-----------------------------------------------------------------------------
Dequeueing
-----------------------------------------------------------------------------*/

static bool
payz_tester_command_get_(const tal_t *ctx,
			 struct list_head *queue,
			 char **buffer,
			 jsmntok_t **tok)
{
	struct payz_tester_command_qentry *qe;
	qe = list_pop(queue,
		      struct payz_tester_command_qentry,
		      list);
	if (!qe)
		return false;
	*buffer = tal_steal(ctx, qe->buffer);
	*tok = tal_steal(ctx, qe->toks);
	tal_free(qe);
	return true;
}

bool
payz_tester_command_get_response(const tal_t *ctx,
				 struct payz_tester_command *command,
				 char **buffer,
				 jsmntok_t **tok)
{
	return payz_tester_command_get_(ctx, &command->responses,
					buffer, tok);
}

bool
payz_tester_command_get_notif(const tal_t *ctx,
			      struct payz_tester_command *command,
			      char **buffer,
			      jsmntok_t **tok)
{
	return payz_tester_command_get_(ctx, &command->notifs,
					buffer, tok);
}

/*-----------------------------------------------------------------------------
Sending Commands
-----------------------------------------------------------------------------*/

void payz_tester_command_send(struct payz_tester_command *command,
			      struct timemono start,
			      u64 id,
			      const char *method,
			      const char *params)
{
	char *request;
	size_t nrequest;

	request = tal_fmt(tmpctx,
			  "{\"jsonrpc\": \"2.0\", \"id\": %"PRIu64", "
			  " \"method\": \"%s\", \"params\": %s}",
			  id, method, params);
	nrequest = strlen(request);

	write_timed(command->to_stdin, request, nrequest,
		    time_mono(), command->timeout);
}

/*-----------------------------------------------------------------------------
Timed Write
-----------------------------------------------------------------------------*/

static void write_timed(int fd, const void *vbuffer, size_t len,
			struct timemono start, struct timerel timeout)
{
	const char *buffer = (const char *) vbuffer;
	ssize_t nwrite;

	struct pollfd pfd;
	int poll_res;

	while (len > 0) {
		/* Have we timed out?  */
		if (time_greater(timemono_since(start), timeout))
			errx(1, "plugin stdin: Timed out.");

		/* Poll for 20 milliseconds.  */
		pfd.fd = fd;
		pfd.events = POLLOUT;
		poll_res = poll(&pfd, 1, 20);
		if (poll_res < 0 && errno != EINTR)
			err(1, "plugin stdin: poll(%d)", fd);
		/* Can we write?*/
		if (poll_res <= 0)
			continue;

		nwrite = write(fd, buffer, len);
		if (nwrite < 0 && errno != EINTR)
			err(1, "plugin stdin: write(%d)", fd);
		if (nwrite > 0) {
			assert(nwrite <= len);
			buffer += nwrite;
			len -= nwrite;
		}
	}
}
