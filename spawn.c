/*
 * spawn.c - Lua configuration management
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/** awesome core API
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @module awesome
 */

/** For some reason the application aborted startup
 * @param arg Table which only got the "id" key set
 * @signal spawn::canceled
 */

/** When one of the fields from the @{spawn::initiated} table changes
 * @param arg Table which describes the spawn event
 * @signal spawn::change
 */

/** An application finished starting
 * @param arg Table which only got the "id" key set
 * @signal spawn::completed
 */

/** When a new client is beginning to start
 * @param arg Table which describes the spawn event
 * @signal spawn::initiated
 */

/** An application started a spawn event but didn't start in time.
 * @param arg Table which only got the "id" key set
 * @signal spawn::timeout
 */

#include "spawn.h"

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <glib.h>

/** 20 seconds timeout */
#define AWESOME_SPAWN_TIMEOUT 20.0

static int
compare_pids(const void *a, const void *b)
{
    return *(GPid *) a - *(GPid *)b;
}

/** Wrapper for unrefing startup sequence.
 */
static inline void
a_sn_startup_sequence_unref(SnStartupSequence **sss)
{
    return sn_startup_sequence_unref(*sss);
}

DO_ARRAY(SnStartupSequence *, SnStartupSequence, a_sn_startup_sequence_unref)

/** The array of startup sequence running */
static SnStartupSequence_array_t sn_waits;

DO_BARRAY(GPid, pid, DO_NOTHING, compare_pids)

static pid_array_t running_children;

/** Remove a SnStartupSequence pointer from an array and forget about it.
 * \param s The startup sequence to found, remove and unref.
 * \return True if found and removed.
 */
static inline bool
spawn_sequence_remove(SnStartupSequence *s)
{
    for(int i = 0; i < sn_waits.len; i++)
        if(sn_waits.tab[i] == s)
        {
            SnStartupSequence_array_take(&sn_waits, i);
            sn_startup_sequence_unref(s);
            return true;
        }
    return false;
}

static gboolean
spawn_monitor_timeout(gpointer sequence)
{
    if(spawn_sequence_remove(sequence))
    {
         signal_t *sig = signal_array_getbyid(&global_signals,
                                              a_strhash((const unsigned char *) "spawn::timeout"));
         if(sig)
         {
             /* send a timeout signal */
             lua_State *L = globalconf_get_lua_State();
             lua_createtable(L, 0, 2);
             lua_pushstring(L, sn_startup_sequence_get_id(sequence));
             lua_setfield(L, -2, "id");
             foreach(func, sig->sigfuncs)
             {
                 lua_pushvalue(L, -1);
                 luaA_object_push(L, (void *) *func);
                 luaA_dofunction(L, 1, 0);
             }
             lua_pop(L, 1);
         }
    }
    sn_startup_sequence_unref(sequence);
    return FALSE;
}

static void
spawn_monitor_event(SnMonitorEvent *event, void *data)
{
    lua_State *L = globalconf_get_lua_State();
    SnStartupSequence *sequence = sn_monitor_event_get_startup_sequence(event);
    SnMonitorEventType event_type = sn_monitor_event_get_type(event);

    lua_createtable(L, 0, 2);
    lua_pushstring(L, sn_startup_sequence_get_id(sequence));
    lua_setfield(L, -2, "id");

    const char *event_type_str = NULL;

    switch(event_type)
    {
      case SN_MONITOR_EVENT_INITIATED:
        /* ref the sequence for the array */
        sn_startup_sequence_ref(sequence);
        SnStartupSequence_array_append(&sn_waits, sequence);
        event_type_str = "spawn::initiated";

        /* Add a timeout function so we do not wait for this event to complete
         * for ever */
        g_timeout_add_seconds(AWESOME_SPAWN_TIMEOUT, spawn_monitor_timeout, sequence);
        /* ref the sequence for the callback event */
        sn_startup_sequence_ref(sequence);
        break;
      case SN_MONITOR_EVENT_CHANGED:
        event_type_str = "spawn::change";
        break;
      case SN_MONITOR_EVENT_COMPLETED:
        event_type_str = "spawn::completed";
        break;
      case SN_MONITOR_EVENT_CANCELED:
        event_type_str = "spawn::canceled";
        break;
    }

    /* common actions */
    switch(event_type)
    {
      case SN_MONITOR_EVENT_INITIATED:
      case SN_MONITOR_EVENT_CHANGED:
        {
            const char *s = sn_startup_sequence_get_name(sequence);
            if(s)
            {
                lua_pushstring(L, s);
                lua_setfield(L, -2, "name");
            }

            if((s = sn_startup_sequence_get_description(sequence)))
            {
                lua_pushstring(L, s);
                lua_setfield(L, -2, "description");
            }

            lua_pushinteger(L, sn_startup_sequence_get_workspace(sequence));
            lua_setfield(L, -2, "workspace");

            if((s = sn_startup_sequence_get_binary_name(sequence)))
            {
                lua_pushstring(L, s);
                lua_setfield(L, -2, "binary_name");
            }

            if((s = sn_startup_sequence_get_icon_name(sequence)))
            {
                lua_pushstring(L, s);
                lua_setfield(L, -2, "icon_name");
            }

            if((s = sn_startup_sequence_get_wmclass(sequence)))
            {
                lua_pushstring(L, s);
                lua_setfield(L, -2, "wmclass");
            }
        }
        break;
      case SN_MONITOR_EVENT_COMPLETED:
      case SN_MONITOR_EVENT_CANCELED:
        spawn_sequence_remove(sequence);
        break;
    }

    /* send the signal */
    signal_t *sig = signal_array_getbyid(&global_signals,
                                         a_strhash((const unsigned char *) event_type_str));

    if(sig)
    {
        foreach(func, sig->sigfuncs)
        {
            lua_pushvalue(L, -1);
            luaA_object_push(L, (void *) *func);
            luaA_dofunction(L, 1, 0);
        }
        lua_pop(L, 1);
    }
}

/** Tell the spawn module that an app has been started.
 * \param c The client that just started.
 * \param startup_id The startup id of the started application.
 */
void
spawn_start_notify(client_t *c, const char * startup_id)
{
    foreach(_seq, sn_waits)
    {
        SnStartupSequence *seq = *_seq;
        bool found = false;
        const char *seqid = sn_startup_sequence_get_id(seq);

        if (A_STRNEQ(seqid, startup_id))
            found = true;
        else
        {
            const char *seqclass = sn_startup_sequence_get_wmclass(seq);
            if (A_STREQ(seqclass, c->class) || A_STREQ(seqclass, c->instance))
                found = true;
            else
            {
                const char *seqbin = sn_startup_sequence_get_binary_name(seq);
                if (A_STREQ_CASE(seqbin, c->class) || A_STREQ_CASE(seqbin, c->instance))
                    found = true;
            }
        }

        if(found)
        {
            sn_startup_sequence_complete(seq);
            break;
        }
    }
}

static void
remove_running_child(GPid pid, gint status, gpointer user_data)
{
    (void) status;
    (void) user_data;

    GPid *pid_in_array = pid_array_lookup(&running_children, &pid);
    if (pid_in_array != NULL) {
        pid_array_remove(&running_children, pid_in_array);
    } else {
        warn("(Partially) unknown child %d exited?!", (int) pid);
    }
}

/** Initialize program spawner.
 */
void
spawn_init(void)
{
    globalconf.sndisplay = sn_xcb_display_new(globalconf.connection, NULL, NULL);

    globalconf.snmonitor = sn_monitor_context_new(globalconf.sndisplay,
                                                  globalconf.default_screen,
                                                  spawn_monitor_event,
                                                  NULL, NULL);
}

void
spawn_handle_reap(const char *arg)
{
    GPid pid = atoll(arg);
    pid_array_insert(&running_children, pid);
    g_child_watch_add((GPid) pid, remove_running_child, NULL);
}

/** Called right before exit, serialise state in case of a restart.
 */
char * const *
spawn_transform_commandline(char **argv)
{
    size_t offset = 0;
    size_t length = 0;
    while(argv[offset] != NULL)
    {
        if(A_STREQ(argv[offset], "--reap"))
            offset += 2;
        else {
            length++;
            offset++;
        }
    }

    length += 2*running_children.len;

    const char ** transformed = p_new(const char *, length+1);
    size_t index = 0;
    offset = 0;
    while(argv[index + offset] != NULL)
    {
        if(A_STREQ(argv[index + offset], "--reap"))
            offset += 2;
        else {
            transformed[index] = argv[index + offset];
            index++;
        }
    }

    foreach(pid, running_children)
    {
        buffer_t buffer;
        buffer_init(&buffer);
        buffer_addf(&buffer, "%d", (int) *pid);

        transformed[index++] = "--reap";
        transformed[index++] = buffer_detach(&buffer);
        buffer_wipe(&buffer);
    }
    transformed[index++] = NULL;

    return (char * const *) transformed;
}

static gboolean
spawn_launchee_timeout(gpointer context)
{
    sn_launcher_context_complete(context);
    sn_launcher_context_unref(context);
    return FALSE;
}

static void
spawn_callback(gpointer user_data)
{
    SnLauncherContext *context = (SnLauncherContext *) user_data;
    setsid();

    if (context)
        sn_launcher_context_setup_child_process(context);
    else
        /* Unset in case awesome was already started with this variable set */
        unsetenv("DESKTOP_STARTUP_ID");
}

/** Parse a command line.
 * \param L The Lua VM state.
 * \param idx The index of the argument that we should parse
 * \return The argv array for the new process.
 */
static gchar **
parse_command(lua_State *L, int idx, GError **error)
{
    gchar **argv = NULL;
    idx = luaA_absindex(L, idx);

    if (lua_isstring(L, idx))
    {
        const char *cmd = luaL_checkstring(L, idx);
        if(!g_shell_parse_argv(cmd, NULL, &argv, error))
            return NULL;
    }
    else if (lua_istable(L, idx))
    {
        size_t i, len = luaA_rawlen(L, idx);

        /* First verify that the table is sane: All integer keys must contain
         * strings. Do this by pushing them all onto the stack.
         */
        for (i = 0; i < len; i++)
        {
            lua_rawgeti(L, idx, i+1);
            if (lua_type(L, -1) != LUA_TSTRING)
                luaL_error(L, "Non-string argument at table index %d", i+1);
        }

        /* From this point on nothing can go wrong and so we can safely allocate
         * memory.
         */
        argv = g_new0(gchar *, len + 1);
        for (i = 0; i < len; i++)
        {
            argv[len - i - 1] = g_strdup(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    else
    {
        luaL_error(L, "Invalid argument to spawn(), expect string or table");
    }

    return argv;
}

/** Callback for when a spawned process exits. */
static void
child_exit_callback(GPid pid, gint status, gpointer user_data)
{
    lua_State *L = globalconf_get_lua_State();
    int exit_callback = GPOINTER_TO_INT(user_data);
    remove_running_child(pid, status, user_data);

    /* 'Decode' the exit status */
    if (WIFEXITED(status)) {
        lua_pushliteral(L, "exit");
        lua_pushinteger(L, WEXITSTATUS(status));
    } else {
        check(WIFSIGNALED(status));
        lua_pushliteral(L, "signal");
        lua_pushinteger(L, WTERMSIG(status));
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, exit_callback);
    luaA_dofunction(L, 2, 0);
    luaA_unregister(L, &exit_callback);
}

/** Spawn a program.
 * The program will be started on the default screen.
 *
 * @tparam string|table cmd The command to launch.
 * @tparam[opt=true] boolean use_sn Use startup-notification?
 * @tparam[opt=false] boolean stdin Return a fd for stdin?
 * @tparam[opt=false] boolean stdout Return a fd for stdout?
 * @tparam[opt=false] boolean stderr Return a fd for stderr?
 * @tparam[opt=nil] function exit_callback Function to call on process exit. The
 * function arguments will be type of exit ("exit" or "signal") and the exit
 * code / the signal number causing process termination.
 * @treturn[1] integer Process ID if everything is OK.
 * @treturn[1] string Startup-notification ID, if `use_sn` is true.
 * @treturn[1] integer stdin, if `stdin` is true.
 * @treturn[1] integer stdout, if `stdout` is true.
 * @treturn[1] integer stderr, if `stderr` is true.
 * @treturn[2] string An error string if an error occured.
 * @function spawn
 */
int
luaA_spawn(lua_State *L)
{
    gchar **argv = NULL;
    bool use_sn = true, return_stdin = false, return_stdout = false, return_stderr = false;
    int stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
    int *stdin_ptr = NULL, *stdout_ptr = NULL, *stderr_ptr = NULL;
    GSpawnFlags flags = 0;
    gboolean retval;
    GPid pid;

    if(lua_gettop(L) >= 2)
        use_sn = luaA_checkboolean(L, 2);
    if(lua_gettop(L) >= 3)
        return_stdin = luaA_checkboolean(L, 3);
    if(lua_gettop(L) >= 4)
        return_stdout = luaA_checkboolean(L, 4);
    if(lua_gettop(L) >= 5)
        return_stderr = luaA_checkboolean(L, 5);
    if (!lua_isnoneornil(L, 6))
    {
        luaA_checkfunction(L, 6);
        flags |= G_SPAWN_DO_NOT_REAP_CHILD;
    }
    if(return_stdin)
        stdin_ptr = &stdin_fd;
    if(return_stdout)
        stdout_ptr = &stdout_fd;
    if(return_stderr)
        stderr_ptr = &stderr_fd;

    GError *error = NULL;
    argv = parse_command(L, 1, &error);
    if(!argv || !argv[0])
    {
        g_strfreev(argv);
        if (error) {
            lua_pushfstring(L, "spawn: parse error: %s", error->message);
            g_error_free(error);
        }
        else
            lua_pushliteral(L, "spawn: There is nothing to execute");
        return 1;
    }

    SnLauncherContext *context = NULL;
    if(use_sn)
    {
        context = sn_launcher_context_new(globalconf.sndisplay, globalconf.default_screen);
        sn_launcher_context_set_name(context, "awesome");
        sn_launcher_context_set_description(context, "awesome spawn");
        sn_launcher_context_set_binary_name(context, argv[0]);
        sn_launcher_context_initiate(context, "awesome", argv[0], globalconf.timestamp);

        /* app will have AWESOME_SPAWN_TIMEOUT seconds to complete,
         * or the timeout function will terminate the launch sequence anyway */
        g_timeout_add_seconds(AWESOME_SPAWN_TIMEOUT, spawn_launchee_timeout, context);
    }

    flags |= G_SPAWN_SEARCH_PATH | G_SPAWN_CLOEXEC_PIPES;
    retval = g_spawn_async_with_pipes(NULL, argv, NULL, flags,
                                      spawn_callback, context, &pid,
                                      stdin_ptr, stdout_ptr, stderr_ptr, &error);
    g_strfreev(argv);
    if(!retval)
    {
        lua_pushstring(L, error->message);
        g_error_free(error);
        if(context)
            sn_launcher_context_complete(context);
        return 1;
    }

    if(flags & G_SPAWN_DO_NOT_REAP_CHILD)
    {
        int exit_callback = LUA_REFNIL;
        /* Only do this down here to avoid leaks in case of errors */
        luaA_registerfct(L, 6, &exit_callback);
        pid_array_insert(&running_children, pid);
        g_child_watch_add(pid, child_exit_callback, GINT_TO_POINTER(exit_callback));
    }

    /* push pid on stack */
    lua_pushinteger(L, pid);

    /* push sn on stack */
    if (context)
        lua_pushstring(L, sn_launcher_context_get_startup_id(context));
    else
        lua_pushnil(L);

    if(return_stdin)
        lua_pushinteger(L, stdin_fd);
    else
        lua_pushnil(L);
    if(return_stdout)
        lua_pushinteger(L, stdout_fd);
    else
        lua_pushnil(L);
    if(return_stderr)
        lua_pushinteger(L, stderr_fd);
    else
        lua_pushnil(L);

    return 5;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
