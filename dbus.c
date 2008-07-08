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
a_dbus_process_request_client(DBusMessage *msg)
{
    int i = 2;
    DBusMessageIter iter;
    const char *m;
    char **path, *arg;
    client_t *c;
    bool b;
    uint64_t i64;
    double d;

    if(!dbus_message_get_path_decomposed(msg, &path))
        return;

    /* path is:
     * /org/awesome/clients/bywhat/id */
    if(!a_dbus_path_check(path, 5)
       || a_strcmp(path[2], "clients"))
        goto bailout;

    /* handle by- */
    switch(a_tokenize(path[3], -1))
    {
      case A_TK_BYNAME:
        for(c = globalconf.clients; c; c = c->next)
            if(!a_strcmp(c->name, path[4]))
                break;
        /* not found, return */
        if(!c)
            goto bailout;
        printf("found\n");
        break;
      default:
        return;
    }

    if(!dbus_message_iter_init(msg, &iter))
    {
        printf("bad init\n");
        dbus_error_free(&err);
        return;
    }

    /* clear stack */
    lua_settop(globalconf.L, 0);

    /* push index function */
    lua_pushcfunction(globalconf.L, luaA_client_newindex);

    /* push the client */
    luaA_client_userdata_new(globalconf.L, c);

    /* get the method name */
    m = dbus_message_get_member(msg);
    
    /* push the method name */
    lua_pushstring(globalconf.L, m);

    do
    {
        switch(dbus_message_iter_get_arg_type(&iter))
        {
          case DBUS_TYPE_STRING:
            dbus_message_iter_get_basic(&iter, &arg);
            lua_pushstring(globalconf.L, arg);
            break;
          case DBUS_TYPE_BOOLEAN:
            dbus_message_iter_get_basic(&iter, &b);
            lua_pushboolean(globalconf.L, b);
            break;
          case DBUS_TYPE_INT16:
          case DBUS_TYPE_UINT16:
          case DBUS_TYPE_INT32:
          case DBUS_TYPE_UINT32:
          case DBUS_TYPE_INT64:
          case DBUS_TYPE_UINT64:
            dbus_message_iter_get_basic(&iter, &i64);
            lua_pushnumber(globalconf.L, i64);
            break;
          case DBUS_TYPE_DOUBLE:
            dbus_message_iter_get_basic(&iter, &d);
            lua_pushnumber(globalconf.L, d);
            break;
          default:
            break;
        }
        i++;
    }
    while(dbus_message_iter_next(&iter));

    lua_pcall(globalconf.L, i, 0, 0);
    printf("pcall\n");

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
    const char *n;
    ssize_t len;

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
        else
        {
            /* get the interface */
            n = dbus_message_get_interface(msg);
            len = a_strlen(n);

            /* check destination: at least has sizeof(org.awesome.)
             * and match org.awesome */
            if(len > 12 && !a_strncmp(n, "org.awesome", 11))
            {
                /* n is now after at the end of 'org.awesome.' */
                n += 12;

                /* we assume that after the dot there's a class name, like
                 * widget, client, etc… */
                switch(a_tokenize(n, -1))
                {
                  case A_TK_CLIENT:
                    a_dbus_process_request_client(msg);
                    break;
                  default:
                    break;
                }
            }
        }

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
        warn("DBus system bus connection failed: %s", err.message);
        dbus_connection = NULL;
        dbus_error_free(&err);
        return false;
    }

    dbus_connection_set_exit_on_disconnect(dbus_connection, FALSE);

    ret = dbus_bus_request_name(dbus_connection, "org.awesome", 0, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to request DBus name: %s", err.message);
        a_dbus_cleanup();
        return false;
    }

    if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        warn("not primary DBus name owner");
        a_dbus_cleanup();
        return false;
    }

    if(!dbus_connection_get_unix_fd(dbus_connection, &fd))
    {
        warn("cannot get DBus connection file descriptor");
        a_dbus_cleanup();
        return false;
    }

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
