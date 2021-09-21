#include"rpc.h"
#include<assert.h>
#include<ccan/err/err.h>
#include<ccan/intmap/intmap.h>
#include<ccan/list/list.h>
#include<ccan/strmap/strmap.h>
#include<ccan/tal/str/str.h>
#include<common/json.h>
#include<common/utils.h>
#include<errno.h>
#include<poll.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/un.h>
#include<unistd.h>

/*-----------------------------------------------------------------------------
Object Definitions and Forward Declarations
-----------------------------------------------------------------------------*/

static const char *lightning_rpc = "lightning-rpc";

/* Sub-object that handles the RPC response queues.  */
struct payz_tester_rpc_queues;
/* Sub-object that handles a single RPC socket.  */
struct payz_tester_rpc_conn;

/* Common object for RPC responses.  */
struct payz_tester_rpc_response {
	/* True if an error.  */
	bool error;
	/* Nul-terminated JSON string.
	 * This either is the actual result if !error,
	 * or the data field if error.
	 */
	char *data;
	/* Nul-terminated JSON string, the string message
	 * if error, NULL if !error.
	 */
	char *message;
	/* Error code if error, 0 if !error.  */
	errcode_t code;

	/* Used for queue.  */
	struct list_node queue;
};

/* The actual RPC handler.  */
struct payz_tester_rpc {
	/* The timeout to respect in case we would block.  */
	struct timerel timeout;
	
	/* Actual directory.  */
	char *dir;

	/* File descriptors used.
	 *
	 * fd[0] is always the socket we are listening on.
	 */
	int *fds;

	/* RPC response queues.  */
	struct payz_tester_rpc_queues *queues;
	/* Individual RPC connections, key is the fd.  */
	SINTMAP(struct payz_tester_rpc_conn *) conns;
};

/* Utility to get current directory in a tal-allocated string.  */
static char *tal_getcwd(const tal_t *ctx);

/* Utility to write data to an fd, making sure not to take too long.  */
static void write_timed(int fd, const void *buffer, size_t size,
			struct timemono start, struct timerel timeout);

/** payz_tester_rpc_queues_new
 *
 * @brief Construct an RPC response queues.
 *
 * @param ctx - owner of the RPC response queues.
 */
static struct payz_tester_rpc_queues *
payz_tester_rpc_queues_new(const tal_t *ctx);

/** payz_tester_rpc_queues_pop
 *
 * @brief Dequeue an RPC response for the given
 * method, or return NULL if not present.
 *
 * @param ctx - the owner of the RPC response to be
 * returned.
 * @param queues - the RPC response queues object to
 * query.
 * @param method - the name of the RPC request.
 *
 * @return - a response to that RPC request, or
 * NULL if one is not yet queued.
 */
static struct payz_tester_rpc_response *
payz_tester_rpc_queues_pop(const tal_t *ctx,
			   struct payz_tester_rpc_queues *queues,
			   const char *method);

/** payz_tester_rpc_queues_push
 *
 * @brief Enqueue an RPC response for the given
 * method.
 *
 * @param queues - the RPC response queues object to
 * add to.
 * @param method - the name of the RPC request.
 * @param response - the response object, can be
 * take().
 */
static void
payz_tester_rpc_queues_push(struct payz_tester_rpc_queues *queues,
			    const char *method,
			    struct payz_tester_rpc_response *response TAKES);

/** payz_tester_rpc_queues_has_outgoing
 *
 * @brief Determine if the queue is empty or not.
 *
 * @param queues - the RPC response queues object to
 * check.
 *
 * @return - true if there is at least one queued
 * response.
 */
static bool
payz_tester_rpc_queues_has_outgoing(
		const struct payz_tester_rpc_queues *queues);

/** payz_tester_rpc_conn_new
 *
 * @brief Construct an RPC connection object.
 *
 * @param ctx - owner of the RPC connection.
 * @param queues - the RPC response queues object to get
 * from.
 * @param fd - the file descriptor of the RPC connection.
 * @param timeout - the timeout to use for connections.
 */
static struct payz_tester_rpc_conn *
payz_tester_rpc_conn_new(const tal_t *ctx,
			 struct payz_tester_rpc_queues *queues,
			 int fd,
			 struct timerel timeout);

/** payz_tester_rpc_conn_feed
 *
 * @brief Feed a buffer of read data into an RPC connection
 * object.
 *
 * @param conn - the RPC connection object.
 * @param buffer - a tal-allocated array containing the
 * data read in, the size of the array is exactly the
 * data read in.
 * @param start - the start time to use when measuring
 * timeouts.
 */
static void
payz_tester_rpc_conn_feed(struct payz_tester_rpc_conn *conn,
			  char *buffer TAKES,
			  struct timemono start);

/** payz_tester_rpc_conn_tryrespond
 *
 * @brief Give a response to any pending request for a particular
 * method.
 *
 * @param conn - the RPC connection object.
 * @param method - the method the response is for.
 * @param response - the RPC response to give, if there is a
 * pending request for the given method.
 * @param start - the start time to measure timeouts from.
 * @param timeout - the maximum time to write a response to
 * the plugin.
 *
 * @return - true if there was a pending request, which was
 * resolved by this response.
 * False if there is no existing request.
 */
static bool
payz_tester_rpc_conn_tryrespond(struct payz_tester_rpc_conn *conn,
				const char *method,
				struct payz_tester_rpc_response *response,
				struct timemono start,
				struct timerel timeout);

/** payz_tester_rpc_conn_has_incoming
 *
 * @brief check if there are pending incoming RPC requests
 * from the plugin.
 *
 * @param conn - the RPC connection object to query.
 *
 * @return - true if there is are any pending requests.
 */
static bool
payz_tester_rpc_conn_has_incoming(const struct payz_tester_rpc_conn *conn);

/** payz_tester_rpc_add_response_obj
 *
 * @brief Provide a response, given a constructed response
 * object.
 *
 * @param rpc - the RPC emulator object.
 * @param method - the RPC method to respond to.
 * @param response - the response to add, can be take().
 */
static void
payz_tester_rpc_add_response_obj(struct payz_tester_rpc *rpc,
				 const char *method,
				 struct payz_tester_rpc_response *response
					TAKES);

/** payz_tester_rpc_resolve
 *
 * @brief Resolve an RPC request and send the response via
 * the given fd.
 *
 * @param fd - the file decriptor to write to.
 * @param id - the id of the request.
 * @param response - the response to give.
 * @param start - the start time to measure timeouts from.
 * @param timeout - the maximum time to write to the fd.
 */
static void
payz_tester_rpc_resolve(int fd,
			u64 id,
			struct payz_tester_rpc_response *response,
			struct timemono start,
			struct timerel timeout);

/*-----------------------------------------------------------------------------
Construction
-----------------------------------------------------------------------------*/

static void payz_tester_rpc_destructor(struct payz_tester_rpc *rpc);

struct payz_tester_rpc *payz_tester_rpc_new(const tal_t *ctx,
					    struct timerel timeout)
{
	struct payz_tester_rpc *rpc;
	int res;

	char *origdir;

	int socketfd;
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));

	rpc = tal(ctx, struct payz_tester_rpc);
	rpc->timeout = timeout;
	rpc->dir = mkdtemp(tal_strdup(rpc, "/tmp/payz_tester_rpc.XXXXXX"));
	if (!rpc->dir)
		err(1, "mkdtemp(\"/tmp/payz_tester_rpc.XXXXXX\")");
	rpc->fds = tal_arr(rpc, int, 0);
	rpc->queues = payz_tester_rpc_queues_new(rpc);
	sintmap_init(&rpc->conns);
	tal_add_destructor(rpc, &payz_tester_rpc_destructor);

	/* Change directory to rpc dir.
	 * This avoids storage length issues in the sun_path field.  */
	origdir = tal_getcwd(tmpctx);
	res = chdir(rpc->dir);
	if (res < 0)
		err(1, "chdir(\"%s\")", rpc->dir);

	/* Open the socket.  */
	socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socketfd < 0)
		err(1, "scoket(AF_UNIX, SOCK_STREAM, 0)");

	/* Create the socket file.  */
	strcpy(addr.sun_path, lightning_rpc);
	addr.sun_family = AF_UNIX;
	/* This should not fail, we should have a unique directory.  */
	if (0 > bind(socketfd, (struct sockaddr *) &addr, sizeof(addr)))
		err(1, "bind(%d, \"%s\")", socketfd, addr.sun_path);
	/* Set up listen.  */
	if (0 > listen(socketfd, 120))
		err(1, "listen(%d, 120)", socketfd);
	/* Add socket to array.  */
	tal_arr_expand(&rpc->fds, socketfd);

	/* Socket creation done, return to original directory.  */
	chdir(origdir);

	return rpc;
}

static void payz_tester_rpc_destructor(struct payz_tester_rpc *rpc)
{
	size_t i;

	/* Close all the fds.  */
	for (i = 0; i < tal_count(rpc->fds); ++i) {
		close(rpc->fds[i]);
	}
	/* Deeply delete the tmpdir.  */
	(void) system(tal_fmt(tmpctx, "rm -rf '%s'", rpc->dir));
	/* intmap uses malloc, clean it up.  */
	sintmap_clear(&rpc->conns);
}

/*-----------------------------------------------------------------------------
Accessors
-----------------------------------------------------------------------------*/

const char *payz_tester_rpc_dir(const struct payz_tester_rpc *rpc)
{
	return rpc->dir;
}
const int *payz_tester_rpc_fds(const struct payz_tester_rpc *rpc)
{
	return rpc->fds;
}

/*-----------------------------------------------------------------------------
Socket Reads
-----------------------------------------------------------------------------*/

static void payz_tester_rpc_listenfd(struct payz_tester_rpc *rpc,
				     struct timemono start,
				     int fd);
static void payz_tester_rpc_fd(struct payz_tester_rpc *rpc,
			       struct timemono start,
			       int fd);
void payz_tester_rpc_check(struct payz_tester_rpc *rpc)
{
	struct timemono start = time_mono();

	struct pollfd *polls;
	int res;

	size_t i;

	/* Go through all the FDs we have and set up poll.  */
	polls = tal_arr(tmpctx, struct pollfd, 0);
	for (i = 0; i < tal_count(rpc->fds); ++i) {
		struct pollfd poll;
		poll.fd = rpc->fds[i];
		poll.events = POLLIN;
		tal_arr_expand(&polls, poll);
	}
	/* Just do a quick poll check, 0 timeout.  */
	do {
		res = poll(polls, tal_count(polls), 0);
	} while (res < 0 && errno == EINTR);
	if (res < 0)
		err(1, "payz_tester_rpc: poll");
	/* Check which ones need a read.  */
	for (i = 0; i < tal_count(polls); ++i) {
		/* Skip non-events.  */
		if (polls[i].revents == 0)
			continue;
		/* The first fd is the one where we listen for
		 * new connections, treat is specially.
		 * These can cause changes to the rpc->fds array,
		 * so use the polls array to keep track of the
		 * fd in question.
		 */
		if (i == 0)
			payz_tester_rpc_listenfd(rpc, start, polls[i].fd);
		else
			payz_tester_rpc_fd(rpc, start, polls[i].fd);
	}
}

static void payz_tester_rpc_listenfd(struct payz_tester_rpc *rpc,
				     struct timemono start,
				     int fd)
{
	int newfd;
	struct payz_tester_rpc_conn *conn;

	do {
		newfd = accept(fd, NULL, NULL);
	} while (newfd < 0 && errno == EINTR);
	if (newfd < 0)
		err(1, "payz_tester_rpc: accept");

	/* Add it to the array of fds.  */
	tal_arr_expand(&rpc->fds, newfd);
	/* Create a new connection object.  */
	conn = payz_tester_rpc_conn_new(rpc, rpc->queues, fd, rpc->timeout);
	sintmap_add(&rpc->conns, fd, conn);
}

static void payz_tester_rpc_fd(struct payz_tester_rpc *rpc,
			       struct timemono start,
			       int fd)
{
	char *buffer;
	size_t offset;

	struct pollfd pfd;
	bool readable;

	size_t to_read;
	ssize_t actual_read;

	int res;

	size_t i;

	struct payz_tester_rpc_conn *conn;

	/* Look it up.  */
	conn = sintmap_get(&rpc->conns, fd);
	assert(conn);

	/* Slurp as much data as possible.  */
	buffer = tal_arr(tmpctx, char, 4096);
	offset = 0;
	readable = true;
	do {
		to_read = tal_count(buffer) - offset;
		do {
			actual_read = read(fd, buffer + offset, to_read);
		} while (actual_read < 0 && errno == EINTR);
		if (actual_read < 0)
			err(1, "payz_tester_rpc: read(%d)", fd);

		offset += actual_read;
		if (offset == tal_count(buffer))
			tal_resize(&buffer, tal_count(buffer) + 4096);

		if (actual_read == to_read) {
			/* There may be more to read... */
			pfd.fd = fd;
			pfd.events = POLLIN;
			do {
				res = poll(&pfd, 1, 0);
			} while (res < 0 && errno == EINTR);
			if (res < 0)
				err(1, "payz_tester_rpc: poll(%d)", fd);
			readable = (pfd.events != 0);
		} else
			readable = false;
	} while (readable);
	/* Resize the buffer to exactly the read bytes.  */
	tal_resize(&buffer, offset);

	/* Did we read 0 bytes?
	 * That implies EOF.
	 */
	if (offset == 0) {
		/* Look for the fd in the fds array and remove it.  */
		for (i = 0; i < tal_count(rpc->fds); ++i) {
			if (rpc->fds[i] == fd)
				break;
		}
		if (i < tal_count(rpc->fds))
			tal_arr_remove(&rpc->fds, i);
		/* Delete it from the conns map.  */
		sintmap_del(&rpc->conns, fd);
		tal_free(conn);
		/* Close the fd.  */
		close(fd);
		return;
	}

	/* Feed the data to the connection.  */
	payz_tester_rpc_conn_feed(conn, take(buffer), start);
}

/*-----------------------------------------------------------------------------
Query and management API
-----------------------------------------------------------------------------*/

bool payz_tester_rpc_has_incoming(const struct payz_tester_rpc *rpc)
{
	struct payz_tester_rpc_conn *conn;
	sintmap_index_t index;

	/* Go through each connection and ask it if it has incoming.  */
	for (conn = sintmap_first(&rpc->conns, &index);
	     conn;
	     conn = sintmap_after(&rpc->conns, &index)) {
		if (payz_tester_rpc_conn_has_incoming(conn))
			return true;
	}
	return false;
}

bool payz_tester_rpc_has_outgoing(const struct payz_tester_rpc *rpc)
{
	return payz_tester_rpc_queues_has_outgoing(rpc->queues);
}

void payz_tester_rpc_clear(struct payz_tester_rpc *rpc)
{
	/* Just recreate the queues object.  */
	tal_free(rpc->queues);
	rpc->queues = payz_tester_rpc_queues_new(rpc);
}

/*-----------------------------------------------------------------------------
Response queueing API
-----------------------------------------------------------------------------*/

void payz_tester_rpc_add_response(struct payz_tester_rpc *rpc,
				  const char *method TAKES,
				  const char *data TAKES)
{
	struct payz_tester_rpc_response *response;

	if (taken(method))
		tal_steal(tmpctx, method);

	response = tal(NULL, struct payz_tester_rpc_response);
	response->error = false;
	response->data = tal_strdup(response, data);
	response->message = NULL;
	response->code = 0;

	payz_tester_rpc_add_response_obj(rpc, method, take(response));
}

void payz_tester_rpc_add_error(struct payz_tester_rpc *rpc,
			       const char *method TAKES,
			       errcode_t code,
			       const char *message TAKES,
			       const char *data TAKES)
{
	struct payz_tester_rpc_response *response;

	if (taken(method))
		tal_steal(tmpctx, method);

	response = tal(NULL, struct payz_tester_rpc_response);
	response->error = true;
	response->data = tal_strdup(response, data);
	response->message = tal_strdup(response, message);
	response->code = code;

	payz_tester_rpc_add_response_obj(rpc, method, take(response));
}

/* Actual core function.  */
static void
payz_tester_rpc_add_response_obj(struct payz_tester_rpc *rpc,
				 const char *method,
				 struct payz_tester_rpc_response *response
					TAKES)
{
	struct timemono start = time_mono();

	struct payz_tester_rpc_conn *conn;
	sintmap_index_t index;

	/* Go through each connection and ask it if it has a request
	 * waiting for the response.
	 */
	for (conn = sintmap_first(&rpc->conns, &index);
	     conn;
	     conn = sintmap_after(&rpc->conns, &index)) {
		if (payz_tester_rpc_conn_tryrespond(conn, method, response,
						    start, rpc->timeout)) {
			/* Someone has handled it, end at this point.  */
			if (taken(response))
				tal_free(response);
			return;
		}
	}

	/* Nobody is waiting for a response, add it to the queues
	 * object instead.
	 */
	payz_tester_rpc_queues_push(rpc->queues, method, response);
}

/*-----------------------------------------------------------------------------
Individual RPC Connections
-----------------------------------------------------------------------------*/

/** struct payz_tester_rpc_request
 *
 * @brief An individual request that has not been fulfilled yet.
 */
struct payz_tester_rpc_request {
	struct list_node list;

	u64 id;
};

struct payz_tester_rpc_conn {
	/* The queues object from which we will get responses from.  */
	struct payz_tester_rpc_queues *queues;

	/* File descriptor of the socket.  */
	int fd;
	/* Timeouts for writes.  */
	struct timerel timeout;
	/* Mapping from method names to list-heads of requests.  */
	STRMAP(struct list_head *) requests;

	/* Parser state.  */
	jsmn_parser parser;
	char *buffer;
	jsmntok_t *toks;
};

static void
payz_tester_rpc_conn_destructor(struct payz_tester_rpc_conn *conn);
static void
payz_tester_rpc_conn_add_request(struct payz_tester_rpc_conn *conn,
				 const char *method,
				 u64 id);
static void
payz_tester_rpc_conn_process(struct payz_tester_rpc_conn *conn,
			     const char *buffer,
			     const jsmntok_t *tok,
			     struct timemono start);

static struct payz_tester_rpc_conn *
payz_tester_rpc_conn_new(const tal_t *ctx,
			 struct payz_tester_rpc_queues *queues,
			 int fd,
			 struct timerel timeout)
{
	struct payz_tester_rpc_conn *conn;

	conn = tal(ctx, struct payz_tester_rpc_conn);
	conn->queues = queues;
	conn->fd = fd;
	conn->timeout = timeout;
	strmap_init(&conn->requests);
	jsmn_init(&conn->parser);
	conn->buffer = tal_arr(conn, char, 0);
	conn->toks = toks_alloc(conn);
	tal_add_destructor(conn, &payz_tester_rpc_conn_destructor);

	return conn;
}

static void
payz_tester_rpc_conn_destructor(struct payz_tester_rpc_conn *conn)
{
	/* strmap uses malloc, delete explicitly.  */
	strmap_clear(&conn->requests);
}

static void
payz_tester_rpc_conn_feed(struct payz_tester_rpc_conn *conn,
			  char *buffer TAKES,
			  struct timemono start)
{
	bool complete;

	if (taken(buffer))
		tal_steal(tmpctx, buffer);
	tal_expand(&conn->buffer, buffer, tal_count(buffer));

	do {
		if (!json_parse_input(&conn->parser, &conn->toks,
				      conn->buffer, tal_count(conn->buffer),
				      &complete))
			errx(1, "RPC: malformed JSON text in request: %.*s",
			     (int) tal_count(conn->buffer), conn->buffer);

		if (!complete)
			return;

		/* json_parse_input returns a single-token array if
		 * only whitespace left.  */
		if (tal_count(conn->toks) == 1) {
			tal_resize(&conn->buffer, 0);
			jsmn_init(&conn->parser);
			toks_reset(conn->toks);
			return;
		}

		/* Process a single request.  */
		payz_tester_rpc_conn_process(conn, conn->buffer, conn->toks,
					     start);

		/* Consume one request from the buffer and reset the
		 * parser.
		 */
		memmove(conn->buffer, conn->buffer + conn->toks[0].end,
			tal_count(conn->buffer) - conn->toks[0].end);
		tal_resize(&conn->buffer,
			   tal_count(conn->buffer) - conn->toks[0].end);
		jsmn_init(&conn->parser);
		toks_reset(conn->toks);

	} while (tal_count(conn->buffer) != 0);
}

static void
payz_tester_rpc_conn_process(struct payz_tester_rpc_conn *conn,
			     const char *buffer,
			     const jsmntok_t *tok,
			     struct timemono start)
{
	const char *error;

	char *method;
	u64 id;

	struct payz_tester_rpc_response *response;

	error = json_scan(tmpctx, buffer, tok,
			  "{method:%, id:%}",
			  JSON_SCAN_TAL(tmpctx, json_strdup, &method),
			  JSON_SCAN(json_to_u64, &id));
	if (error)
		errx(1, "Invalid JSON-RPC request: %s", error);

	response = payz_tester_rpc_queues_pop(tmpctx, conn->queues, method);

	if (!response)
		payz_tester_rpc_conn_add_request(conn, method, id);
	else
		payz_tester_rpc_resolve(conn->fd, id, take(response),
					start, conn->timeout);
}

static void
payz_tester_rpc_conn_add_request(struct payz_tester_rpc_conn *conn,
				 const char *method,
				 u64 id)
{
	struct payz_tester_rpc_request *request;
	struct list_head *request_list;

	request = tal(conn, struct payz_tester_rpc_request);
	request->id = id;

	request_list = strmap_get(&conn->requests, method);
	if (!request_list) {
		char *key;

		key = tal_strdup(conn, method);

		request_list = tal(conn, struct list_head);
		list_head_init(request_list);
		strmap_add(&conn->requests, key, request_list);
	}

	list_add_tail(request_list, &request->list);
}

static bool
payz_tester_rpc_conn_tryrespond(struct payz_tester_rpc_conn *conn,
				const char *method,
				struct payz_tester_rpc_response *response,
				struct timemono start,
				struct timerel timeout)
{
	struct payz_tester_rpc_request *request;
	struct list_head *request_list;

	/* Any pending request?  */
	request_list = strmap_get(&conn->requests, method);
	if (!request_list || list_empty(request_list))
		return false;

	/* Pop off the top.  */
	request = list_pop(request_list, struct payz_tester_rpc_request, list);
	tal_steal(tmpctx, request);

	payz_tester_rpc_resolve(conn->fd, request->id, response,
				start, timeout);
	return true;
}

static bool
payz_tester_rpc_conn_has_incoming_scan(const char *method UNUSED,
				       struct list_head *request_list,
				       bool *found);

static bool
payz_tester_rpc_conn_has_incoming(const struct payz_tester_rpc_conn *conn)
{
	bool found = false;
	strmap_iterate(&conn->requests,
		       &payz_tester_rpc_conn_has_incoming_scan,
		       &found);
	return found;
}

static bool
payz_tester_rpc_conn_has_incoming_scan(const char *method UNUSED,
				       struct list_head *request_list,
				       bool *found)
{
	/* If the list is empty, keep scanning.  */
	if (list_empty(request_list))
		return true;
	/* Otherwise we found a pending incoming request, stop
	 * scanning.  */
	*found = true;
	return false;
}

/*-----------------------------------------------------------------------------
Queue For RPC Responses
-----------------------------------------------------------------------------*/

struct payz_tester_rpc_queues {
	/* Basic core.  */
	STRMAP(struct list_head *) responses;
};

static void
payz_tester_rpc_queues_destructor(struct payz_tester_rpc_queues *queues);

static struct payz_tester_rpc_queues *
payz_tester_rpc_queues_new(const tal_t *ctx)
{
	struct payz_tester_rpc_queues *queues;

	queues = tal(ctx, struct payz_tester_rpc_queues);
	strmap_init(&queues->responses);
	tal_add_destructor(queues, &payz_tester_rpc_queues_destructor);

	return queues;
}

static void
payz_tester_rpc_queues_destructor(struct payz_tester_rpc_queues *queues)
{
	/* strmap uses malloc, clear it here.  */
	strmap_clear(&queues->responses);
}

static struct payz_tester_rpc_response *
payz_tester_rpc_queues_pop(const tal_t *ctx,
			   struct payz_tester_rpc_queues *queues,
			   const char *method)
{
	struct list_head *response_list;
	struct payz_tester_rpc_response *response;

	response_list = strmap_get(&queues->responses, method);
	if (!response_list || list_empty(response_list))
		return NULL;

	response = list_pop(response_list,
			    struct payz_tester_rpc_response, queue);
	return tal_steal(ctx, response);
}

static void
payz_tester_rpc_queues_push(struct payz_tester_rpc_queues *queues,
			    const char *method,
			    struct payz_tester_rpc_response *response TAKES)
{
	struct list_head *response_list;

	/* We need to copy deeply, including strings, so we cannot
	 * just use tal_dup here.
	 */
	if (!taken(response)) {
		struct payz_tester_rpc_response *new_response;

		new_response = tal(queues, struct payz_tester_rpc_response);
		new_response->error = response->error;
		new_response->data = tal_strdup(new_response,
						response->data);
		new_response->message = tal_strdup(new_response,
						   response->message);
		new_response->code = response->code;

		response = new_response;
	}

	response_list = strmap_get(&queues->responses, method);
	if (!response_list) {
		char *key;

		key = tal_strdup(queues, method);

		response_list = tal(queues, struct list_head);
		list_head_init(response_list);
		strmap_add(&queues->responses, key, response_list);
	}

	list_add_tail(response_list, &response->queue);
	tal_steal(response_list, response);
}

static bool
payz_tester_rpc_queues_has_outgoing_scan(
		const char *method UNUSED,
		struct list_head *response_list,
		bool *found);

static bool
payz_tester_rpc_queues_has_outgoing(
		const struct payz_tester_rpc_queues *queues)
{
	bool found = false;
	strmap_iterate(&queues->responses,
		       &payz_tester_rpc_queues_has_outgoing_scan,
		       &found);
	return found;
}

static bool
payz_tester_rpc_queues_has_outgoing_scan(
		const char *method UNUSED,
		struct list_head *response_list,
		bool *found)
{
	if (list_empty(response_list))
		return true;
	*found = true;
	return false;
}

/*-----------------------------------------------------------------------------
Responding to RPC Requests
-----------------------------------------------------------------------------*/

static void
payz_tester_rpc_resolve(int fd,
			u64 id,
			struct payz_tester_rpc_response *response,
			struct timemono start,
			struct timerel timeout)
{
	char *out;
	size_t size;

	assert(response);

	if (response->error)
		out = tal_fmt(tmpctx,
			      "{\"jsonrpc\": \"2.0\", \"id\": %"PRIu64", "
			      " \"error\": {\"code\": %"PRIerrcode", "
			      "             \"data\": %s, \"message\": %s}}",
			      id, response->code, response->data,
			      response->message);
	else
		out = tal_fmt(tmpctx,
			      "{\"jsonrpc\": \"2.0\", \"id\": %"PRIu64", "
			      " \"result\": %s}",
			      id, response->data);
	/* Omit terminating nul char.  */
	size = tal_count(out) - 1;

	write_timed(fd, out, size, start, timeout);
}

/*-----------------------------------------------------------------------------
Utility
-----------------------------------------------------------------------------*/

static char *tal_getcwd(const tal_t *ctx)
{
	char *cwd = tal_arr(ctx, char, 64);
	char *ret;
	
	for (;;) {
		ret = getcwd(cwd, tal_count(cwd));
		/* Fits?  */
		if (ret)
			/* Let tal_strdup handle making the buffer exactly
			 * the size of the string.
			 */
			return tal_strdup(ctx, take(cwd));
		/* An error other than not-fit-in-buffer?  */
		if (errno != ERANGE)
			err(1, "getcwd");

		/* Make buffer larger.  */
		tal_resize(&cwd, tal_count(cwd) * 2);
	}
}

static void write_timed(int fd, const void *buffer, size_t size,
			struct timemono start, struct timerel timeout)
{
	struct pollfd pfd;
	const char *pbuf = (const char *) buffer;

	int poll_res;
	ssize_t nwrite;

	while (size > 0) {
		/* Have we timed out?  */
		if (time_greater(timemono_since(start), timeout))
			errx(1, "Write of RPC response timed out.");

		/* Poll for 20 milliseconds.  */
		pfd.fd = fd;
		pfd.events = POLLOUT;
		poll_res = poll(&pfd, 1, 20);
		if (poll_res < 0 && errno != EINTR)
			err(1, "RPC response write: poll(%d)", fd);
		/* Can we write?  */
		if (poll_res <= 0)
			continue;

		nwrite = write(fd, pbuf, size);
		if (nwrite < 0 && errno != EINTR)
			err(1, "RPC response write: write(%d)", fd);
		if (nwrite > 0) {
			assert(nwrite <= size);
			pbuf += nwrite;
			size -= nwrite;
		}
	}
}

