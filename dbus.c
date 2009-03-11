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
#include "dbus.h"
#include "client.h"

#ifdef WITH_DBUS

#include <ev.h>
#include <dbus/dbus.h>
#include <unistd.h>
#include <fcntl.h>

#include "widget.h"

extern awesome_t globalconf;

static DBusError err;
static DBusConnection *dbus_connection = NULL;
ev_io dbusio = { .fd = -1 };

static int
a_dbus_message_iter(DBusMessageIter *iter)
{
    int nargs = 0;

    do
    {
        switch(dbus_message_iter_get_arg_type(iter))
        {
          default:
            lua_pushnil(globalconf.L);
            nargs++;
            break;
          case DBUS_TYPE_INVALID:
            break;
          case DBUS_TYPE_VARIANT:
            {
                DBusMessageIter subiter;
                dbus_message_iter_recurse(iter, &subiter);
                a_dbus_message_iter(&subiter);
            }
            nargs++;
            break;
          case DBUS_TYPE_DICT_ENTRY:
            {
                DBusMessageIter subiter;

                /* initialize a sub iterator */
                dbus_message_iter_recurse(iter, &subiter);
                /* create a new table to store the dict */
                a_dbus_message_iter(&subiter);
            }
            nargs++;
            break;
          case DBUS_TYPE_STRUCT:
            {
                /* create a new table to store all the value */
                lua_newtable(globalconf.L);

                DBusMessageIter subiter;
                /* initialize a sub iterator */
                dbus_message_iter_recurse(iter, &subiter);
                /* create a new table to store the dict */
                int n = a_dbus_message_iter(&subiter);

                for(int i = n; i > 0; i--)
                    lua_rawseti(globalconf.L, - i - 1, i);
            }
            nargs++;
            break;
          case DBUS_TYPE_ARRAY:
            {
                int array_type = dbus_message_iter_get_element_type(iter);

                /* create a new table to store all the value */
                lua_newtable(globalconf.L);

                if(dbus_type_is_fixed(array_type))
                {
                    DBusMessageIter sub;
                    dbus_message_iter_recurse(iter, &sub);

                    switch(array_type)
                    {
                      int datalen = 0;
#define DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(type, dbustype) \
                      case dbustype: \
                        { \
                            type *data; \
                            dbus_message_iter_get_fixed_array(&sub, &data, &datalen); \
                            for(int i = 0; i < datalen; i++) \
                            { \
                                lua_pushnumber(globalconf.L, data[i]); \
                                lua_rawseti(globalconf.L, -2, i + 1); \
                            } \
                        } \
                        break;
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(int16_t, DBUS_TYPE_INT16)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(uint16_t, DBUS_TYPE_UINT16)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(int32_t, DBUS_TYPE_INT32)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(uint32_t, DBUS_TYPE_UINT32)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(int64_t, DBUS_TYPE_INT64)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER(uint64_t, DBUS_TYPE_UINT64)
#undef DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER
                    }
                }
                else if(array_type == DBUS_TYPE_DICT_ENTRY)
                {
                    DBusMessageIter subiter;

                    /* initialize a sub iterator */
                    dbus_message_iter_recurse(iter, &subiter);
                    /* get the keys and the values
                     * n is the number of entry in * dict */
                    int n = a_dbus_message_iter(&subiter);

                    for(int i = 0; i < n; i ++)
                        lua_rawset(globalconf.L, - (n * 2) - 1 + i * 2);
                }
                else
                {
                    DBusMessageIter subiter;

                    /* prepare to dig into the array*/
                    dbus_message_iter_recurse(iter, &subiter);

                    /* now iterate over every element of the array */
                    int n = a_dbus_message_iter(&subiter);

                    for(int i = n; i > 0; i--)
                        lua_rawseti(globalconf.L, - i - 1, i);
                }
            }
            nargs++;
            break;
          case DBUS_TYPE_BOOLEAN:
            {
                bool b;
                dbus_message_iter_get_basic(iter, &b);
                lua_pushboolean(globalconf.L, b);
            }
            nargs++;
            break;
          case DBUS_TYPE_BYTE:
            {
                char c;
                dbus_message_iter_get_basic(iter, &c);
                lua_pushlstring(globalconf.L, &c, 1);
            }
            nargs++;
            break;
#define DBUS_MSG_HANDLE_TYPE_NUMBER(type, dbustype) \
          case dbustype: \
            { \
                type ui; \
                dbus_message_iter_get_basic(iter, &ui); \
                lua_pushnumber(globalconf.L, ui); \
            } \
            nargs++; \
            break;
          DBUS_MSG_HANDLE_TYPE_NUMBER(int16_t, DBUS_TYPE_INT16)
          DBUS_MSG_HANDLE_TYPE_NUMBER(uint16_t, DBUS_TYPE_UINT16)
          DBUS_MSG_HANDLE_TYPE_NUMBER(int32_t, DBUS_TYPE_INT32)
          DBUS_MSG_HANDLE_TYPE_NUMBER(uint32_t, DBUS_TYPE_UINT32)
          DBUS_MSG_HANDLE_TYPE_NUMBER(int64_t, DBUS_TYPE_INT64)
          DBUS_MSG_HANDLE_TYPE_NUMBER(uint64_t, DBUS_TYPE_UINT64)
#undef DBUS_MSG_HANDLE_TYPE_NUMBER
          case DBUS_TYPE_STRING:
            {
                char *s;
                dbus_message_iter_get_basic(iter, &s);
                lua_pushstring(globalconf.L, s);
            }
            nargs++;
            break;
        }
    } while(dbus_message_iter_next(iter));

    return nargs;
}

static void
a_dbus_process_request(DBusMessage *msg)
{
    if(globalconf.hooks.dbus == LUA_REFNIL)
        return;

    lua_newtable(globalconf.L);

    switch(dbus_message_get_type(msg))
    {
      case DBUS_MESSAGE_TYPE_SIGNAL:
        lua_pushliteral(globalconf.L, "signal");
        break;
      case DBUS_MESSAGE_TYPE_METHOD_CALL:
        lua_pushliteral(globalconf.L, "method_call");
        break;
      case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        lua_pushliteral(globalconf.L, "method_return");
        break;
      case DBUS_MESSAGE_TYPE_ERROR:
        lua_pushliteral(globalconf.L, "error");
        break;
      default:
        lua_pushliteral(globalconf.L, "unknown");
        break;
    }

    lua_setfield(globalconf.L, -2, "type");

    const char *s = dbus_message_get_interface(msg);
    lua_pushstring(globalconf.L, NONULL(s));
    lua_setfield(globalconf.L, -2, "interface");

    s = dbus_message_get_path(msg);
    lua_pushstring(globalconf.L, NONULL(s));
    lua_setfield(globalconf.L, -2, "path");

    s = dbus_message_get_member(msg);
    lua_pushstring(globalconf.L, NONULL(s));
    lua_setfield(globalconf.L, -2, "member");

    /* + 1 for the table above */
    DBusMessageIter iter;
    int nargs = 1;

    if(dbus_message_iter_init(msg, &iter))
        nargs += a_dbus_message_iter(&iter);

    if(dbus_message_get_no_reply(msg))
        luaA_dofunction(globalconf.L, globalconf.hooks.dbus, nargs, 0);
    else
    {
        int n = lua_gettop(globalconf.L) - nargs;
        luaA_dofunction(globalconf.L, globalconf.hooks.dbus, nargs, LUA_MULTRET);
        n -= lua_gettop(globalconf.L);

        DBusMessage *reply = dbus_message_new_method_return(msg);

        dbus_message_iter_init_append(reply, &iter);

        /* i is negative */
        for(int i = n; i < 0; i += 2)
        {
            /* i is the type name, i+1 the value */
            size_t len;
            const char *type = lua_tolstring(globalconf.L, i, &len);

            if(!type || len != 1)
                break;

            switch(*type)
            {
              case DBUS_TYPE_BOOLEAN:
                {
                    bool b = lua_toboolean(globalconf.L, i + 1);
                    dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &b);
                }
                break;
#define DBUS_MSG_RETURN_HANDLE_TYPE_STRING(dbustype) \
              case dbustype: \
                if((s = lua_tostring(globalconf.L, i + 1))) \
                    dbus_message_iter_append_basic(&iter, dbustype, &s); \
                break;
              DBUS_MSG_RETURN_HANDLE_TYPE_STRING(DBUS_TYPE_STRING)
              DBUS_MSG_RETURN_HANDLE_TYPE_STRING(DBUS_TYPE_BYTE)
#undef DBUS_MSG_RETURN_HANDLE_TYPE_STRING
#define DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(type, dbustype) \
              case dbustype: \
                { \
                    type num = lua_tonumber(globalconf.L, i + 1); \
                    dbus_message_iter_append_basic(&iter, dbustype, &num); \
                } \
                break;
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(int16_t, DBUS_TYPE_INT16)
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint16_t, DBUS_TYPE_UINT16)
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(int32_t, DBUS_TYPE_INT32)
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint32_t, DBUS_TYPE_UINT32)
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(int64_t, DBUS_TYPE_INT64)
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint64_t, DBUS_TYPE_UINT64)
              DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(double, DBUS_TYPE_DOUBLE)
#undef DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER
            }

            lua_remove(globalconf.L, i);
            lua_remove(globalconf.L, i + 1);
        }

        dbus_connection_send(dbus_connection, reply, NULL);
        dbus_message_unref(reply);
    }
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
        else
            a_dbus_process_request(msg);

        dbus_message_unref(msg);

        nmsg++;
    }

    if(nmsg)
        dbus_connection_flush(dbus_connection);
}

static bool
a_dbus_request_name(const char *name)
{
    int ret = dbus_bus_request_name(dbus_connection, name, 0, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to request D-Bus name: %s", err.message);
        return false;
    }

    switch(ret)
    {
      case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
        return true;
      case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
        warn("already primary D-Bus name owner for %s", name);
        return true;
    }
    return false;
}

static bool
a_dbus_release_name(const char *name)
{
    int ret = dbus_bus_release_name(dbus_connection, name, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to release D-Bus name: %s", err.message);
        return false;
    }

    switch(ret)
    {
      case DBUS_RELEASE_NAME_REPLY_NOT_OWNER:
        warn("not primary D-Bus name owner for %s", name);
        return false;
      case DBUS_RELEASE_NAME_REPLY_NON_EXISTENT:
        warn("non existent D-Bus name: %s", name);
        return false;
    }
    return true;
}

bool
a_dbus_init(void)
{
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

    a_dbus_request_name("org.awesome");

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

/** Register a D-Bus name to receive message from.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the name of the D-Bus name to register. Note that
 * org.awesome is registered by default.
 * \lreturn True if everything worked fine, false otherwise.
 */
static int
luaA_dbus_request_name(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_pushboolean(L, a_dbus_request_name(name));
    return 1;
}

/** Release a D-Bus name.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the name of the D-Bus name to unregister.
 * \lreturn True if everything worked fine, false otherwise.
 */
static int
luaA_dbus_release_name(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_pushboolean(L, a_dbus_release_name(name));
    return 1;
}

/** Add a match rule to match messages going through the message bus.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the name of the match rule.
 */
static int
luaA_dbus_add_match(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    dbus_bus_add_match(dbus_connection, name, NULL);
    dbus_connection_flush(dbus_connection);
    return 0;
}

/** Remove a previously added match rule "by value"
 * (the most recently-added identical rule gets removed).
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the name of the match rule.
 */
static int
luaA_dbus_remove_match(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    dbus_bus_remove_match(dbus_connection, name, NULL);
    dbus_connection_flush(dbus_connection);
    return 0;
}

const struct luaL_reg awesome_dbus_lib[] =
{
    { "request_name", luaA_dbus_request_name },
    { "release_name", luaA_dbus_release_name },
    { "add_match", luaA_dbus_add_match },
    { "remove_match", luaA_dbus_remove_match },
    { NULL, NULL }
};

#else /* HAVE_DBUS */

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

static int
luaA_donothing(lua_State *L)
{
    return 0;
}

const struct luaL_reg awesome_dbus_lib[] =
{
    { "request_name", luaA_donothing },
    { "release_name", luaA_donothing },
    { "add_match", luaA_donothing },
    { "remove_match", luaA_donothing },
    { NULL, NULL }
};

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
