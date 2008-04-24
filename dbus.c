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

#include <dbus/dbus.h>

#include "dbus.h"
#include "common/util.h"

static DBusError err;
static DBusConnection *conn = NULL;

static void
a_dbus_process_widget_tell(DBusMessage *req)
{
    int ret;
    char *arg, *path;
    DBusMessageIter iter;
    
    if(!dbus_message_iter_init(req, &iter))
    {
        warn("message has no argument: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    else if(DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&iter))
    {
        warn("argument must be a string");
        dbus_error_free(&err);
        return;
    }
    else
        dbus_message_iter_get_basic(&iter, &arg);

    p_delete(&path);
}

void
a_dbus_process_requests(void)
{
    DBusMessage *msg;
    int nmsg = 0;

    if(!conn && !a_dbus_init())
        return;

    while(true)
    {
        dbus_connection_read_write(conn, 0);

        if(!(msg = dbus_connection_pop_message(conn)))
            break;


        if(dbus_message_is_method_call(msg, "org.awesome.statusbar.widget", "tell"))
            a_dbus_process_widget_tell(msg);
        else if(dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
        {
            a_dbus_cleanup();
            dbus_message_unref(msg);
            return;
        }

        dbus_message_unref(msg);

        nmsg++;
    }

    if(nmsg)
        dbus_connection_flush(conn);
}

bool
a_dbus_init(void)
{
    bool ret;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if(dbus_error_is_set(&err))
    {
        warn("DBus system bus connection failed: %s\n", err.message);

        conn = NULL;

        dbus_error_free(&err);

        return false;
    }

    dbus_connection_set_exit_on_disconnect(conn, FALSE);

    ret = dbus_bus_request_name(conn, "org.awesome", 0, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to request DBus name: %s\n", err.message);

        a_dbus_cleanup();

        return false;
    }

    if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        warn("not primary DBus name owner\n");

        a_dbus_cleanup();

        return false;
    }

    return true;
}

void
a_dbus_cleanup(void)
{
    if(!conn)
        return;

    dbus_error_free(&err);

    /* This is a shared connection owned by libdbus
     * Do not close it, only unref
     */
    dbus_connection_unref(conn);
    conn = NULL;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
