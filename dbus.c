/*
 * dbus.c - awesome dbus support
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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
#include "config.h"

#ifdef WITH_DBUS

#include <ev.h>
#include <dbus/dbus.h>
#include <unistd.h>
#include <fcntl.h>

#include "dbus.h"
#include "widget.h"
#include "client.h"

extern awesome_t globalconf;

static DBusError err;
static DBusConnection *dbus_connection = NULL;
ev_io dbusio = { .fd = -1 };

/** Check a dbus object path format and its number of element.
 * \param path The path.
 * \param nelem The number of element it should have.
 * \return true if the path is ok, false otherwise.
 */
static bool
a_dbus_path_check(char **path, int nelem)
{
    int i;

    for(i = 0; path[i]; i++);
    if(i != nelem)
        return false;
    return (!a_strcmp(path[0], "org") && !a_strcmp(path[1], "awesome"));
}

static void
a_dbus_process_request_do(DBusMessage *msg)
{
    int i;
    DBusMessageIter iter;
    char **path, *cmd;

    if(!dbus_message_get_path_decomposed(msg, &path))
        return;

    /* path is:
     * /org/awesome */
    if(!a_dbus_path_check(path, 2))
        goto bailout;

    if(!dbus_message_iter_init(msg, &iter))
    {
        dbus_error_free(&err);
        goto bailout;
    }
    else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter))
    {
        dbus_error_free(&err);
        goto bailout;
    }
    else
        dbus_message_iter_get_basic(&iter, &cmd);

    luaA_dostring(globalconf.L, cmd);

bailout:
    for(i = 0; path[i]; i++)
        p_delete(&path[i]);
    p_delete(&path);
}

static void
a_dbus_process_requests(EV_P_ ev_io *w, int revents)
{
    DBusMessage *msg;
    int nmsg = 0;

    if(!dbus_connection && !a_dbus_init())
        return;

    while(true)
    {
        dbus_connection_read_write(dbus_connection, 0);

        if(!(msg = dbus_connection_pop_message(dbus_connection)))
            break;

        if(dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
        {
            a_dbus_cleanup();
            dbus_message_unref(msg);
            return;
        }
        else if(dbus_message_is_method_call(msg, "org.awesome", "do"))
            a_dbus_process_request_do(msg);

        dbus_message_unref(msg);

        nmsg++;
    }

    if(nmsg)
        dbus_connection_flush(dbus_connection);
}

bool
a_dbus_init(void)
{
    bool ret;
    int fd;

    dbus_error_init(&err);

    dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if(dbus_error_is_set(&err))
    {
        warn("D-Bus system bus connection failed: %s", err.message);
        dbus_connection = NULL;
        dbus_error_free(&err);
        return false;
    }

    dbus_connection_set_exit_on_disconnect(dbus_connection, FALSE);

    ret = dbus_bus_request_name(dbus_connection, "org.awesome", 0, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to request D-Bus name: %s", err.message);
        a_dbus_cleanup();
        return false;
    }

    if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        warn("not primary D-Bus name owner");
        a_dbus_cleanup();
        return false;
    }

    if(!dbus_connection_get_unix_fd(dbus_connection, &fd))
    {
        warn("cannot get D-Bus connection file descriptor");
        a_dbus_cleanup();
        return false;
    }

    fcntl(fd, F_SETFD, FD_CLOEXEC);

    ev_io_init(&dbusio, a_dbus_process_requests, fd, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ &dbusio);
    ev_unref(EV_DEFAULT_UC);
    return true;
}

void
a_dbus_cleanup(void)
{
    if(!dbus_connection)
        return;

    dbus_error_free(&err);

    if (dbusio.fd >= 0) {
        ev_ref(EV_DEFAULT_UC);
        ev_io_stop(EV_DEFAULT_UC_ &dbusio);
        dbusio.fd = -1;
    }

    /* This is a shared connection owned by libdbus
     * Do not close it, only unref
     */
    dbus_connection_unref(dbus_connection);
    dbus_connection = NULL;
}

#else /* HAVE_DBUS */

#include "dbus.h"

bool
a_dbus_init(void)
{
    return false;
}

void
a_dbus_cleanup(void)
{
    /* empty */
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
