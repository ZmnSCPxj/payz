#ifndef LIGHTNING_PLUGINS_PAYZ_TESTER_SPAWN_H
#define LIGHTNING_PLUGINS_PAYZ_TESTER_SPAWN_H
#include"config.h"
#include<ccan/time/time.h>

/** struct payz_tester_spawn
 *
 * @brief Object that spawns a child process running the plugin
 * code, and manages that child process and its destruction.
 */
struct payz_tester_spawn;

/** payz_tester_spawn_new
 *
 * @brief Constructs a new spawn handler.
 *
 * @desc This is called *without* a tal `ctx` to own the object!
 * The intent is that it will be the *first* ever object
 * constructed by a program, and any intended owner for this
 * object will be constructed later and tal_steal it to get
 * ownership.
 * This is because we do not `exec` into the plugin, we just
 * call the plugin main code, and to minimize any side effects
 * in the child process and keep the execution environment as
 * prstine as feasible, we should avoid dynamic allocations
 * before the fork.
 *
 * The destruction of this object will automatically terminate
 * the plugin and clean up its PID.
 *
 * @param timeout - maximum time to wait when terminating the
 * child on destruction.
 * If the child takes longer than the timeout to close after
 * its stdin has been EOFed, abort() in the parent to fail
 * the test.
 *
 * @return - the newly-constructed object.
 */
struct payz_tester_spawn *payz_tester_spawn_new(struct timerel timeout);

/** payz_tester_spawn_stdin, payz_tester_spawn_stdin
 *
 * @brief Get the stdin/stdout of the plugin.
 * The stdin is a writeable pipe-end, the stdout is a readable
 * pipe-end.
 *
 * @param spawn - the spawn handler to check.
 *
 * @return The stdin/stdout FD.
 * You should not close these; the spawn handler will correctly
 * close them when it is destructed.
 */
int payz_tester_spawn_stdin(const struct payz_tester_spawn *spawn);
int payz_tester_spawn_stdout(const struct payz_tester_spawn *spawn);

#endif /* LIGHTNING_PLUGINS_PAYZ_TESTER_SPAWN_H */
