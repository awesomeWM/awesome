/*
 * dbus.c - awesome dbus support
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

/** awesome D-Bus API
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @release @AWESOME_VERSION@
 * @module dbus
 */

#include "config.h"
#include "dbus.h"

#include <glib.h>

#ifdef WITH_DBUS

#include <dbus/dbus.h>
#include <unistd.h>
#include <fcntl.h>

#include "event.h"
#include "luaa.h"

static DBusConnection *dbus_connection_session = NULL;
static DBusConnection *dbus_connection_system = NULL;
static GSource *session_source = NULL;
static GSource *system_source = NULL;

static signal_array_t dbus_signals;

/** Clean up the D-Bus connection data members
 * \param dbus_connection The D-Bus connection to clean up
 * \param source The D-Bus source
 */
static void
a_dbus_cleanup_bus(DBusConnection *dbus_connection, GSource **source)
{
    if(!dbus_connection)
        return;

    if (*source != NULL)
        g_source_destroy(*source);
    *source = NULL;

    /* This is a shared connection owned by libdbus
     * Do not close it, only unref
     */
    dbus_connection_unref(dbus_connection);
}

/** Iterate through the D-Bus messages counting each or traverse each sub message.
 * \param L The Lua VM state.
 * \param iter The D-Bus message iterator pointer
 * \return The number of arguments in the iterator
 */
static int
a_dbus_message_iter(lua_State *L, DBusMessageIter *iter)
{
    int nargs = 0;

    do
    {
        switch(dbus_message_iter_get_arg_type(iter))
        {
          default:
            lua_pushnil(L);
            nargs++;
            break;
          case DBUS_TYPE_INVALID:
            break;
          case DBUS_TYPE_VARIANT:
            {
                DBusMessageIter subiter;
                dbus_message_iter_recurse(iter, &subiter);
                a_dbus_message_iter(L, &subiter);
            }
            nargs++;
            break;
          case DBUS_TYPE_DICT_ENTRY:
            {
                DBusMessageIter subiter;

                /* initialize a sub iterator */
                dbus_message_iter_recurse(iter, &subiter);
                /* create a new table to store the dict */
                a_dbus_message_iter(L, &subiter);
            }
            nargs++;
            break;
          case DBUS_TYPE_STRUCT:
            {
                DBusMessageIter subiter;
                /* initialize a sub iterator */
                dbus_message_iter_recurse(iter, &subiter);

                int n = a_dbus_message_iter(L, &subiter);

                /* create a new table to store all the value */
                lua_createtable(L, n, 0);
                /* move the table before array elements */
                lua_insert(L, - n - 1);

                for(int i = n; i > 0; i--)
                    lua_rawseti(L, - i - 1, i);
            }
            nargs++;
            break;
          case DBUS_TYPE_ARRAY:
            {
                int array_type = dbus_message_iter_get_element_type(iter);

                if(dbus_type_is_fixed(array_type))
                {
                    DBusMessageIter sub;
                    dbus_message_iter_recurse(iter, &sub);

                    switch(array_type)
                    {
                      int datalen = 0;
#define DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(type, dbustype, pusher) \
                      case dbustype: \
                        { \
                            const type *data; \
                            dbus_message_iter_get_fixed_array(&sub, &data, &datalen); \
                            lua_createtable(L, datalen, 0); \
                            for(int i = 0; i < datalen; i++) \
                            { \
                                pusher(L, data[i]); \
                                lua_rawseti(L, -2, i + 1); \
                            } \
                        } \
                        break;
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(int16_t, DBUS_TYPE_INT16, lua_pushinteger)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(uint16_t, DBUS_TYPE_UINT16, lua_pushinteger)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(int32_t, DBUS_TYPE_INT32, lua_pushinteger)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(uint32_t, DBUS_TYPE_UINT32, lua_pushinteger)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(int64_t, DBUS_TYPE_INT64, lua_pushinteger)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(uint64_t, DBUS_TYPE_UINT64, lua_pushinteger)
                      DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT(double, DBUS_TYPE_DOUBLE, lua_pushnumber)
#undef DBUS_MSG_HANDLE_ARRAY_TYPE_NUMBER_OR_INT
                      case DBUS_TYPE_BYTE:
                        {
                            const char *c;
                            dbus_message_iter_get_fixed_array(&sub, &c, &datalen);
                            lua_pushlstring(L, c, datalen);
                        }
                        break;
                      case DBUS_TYPE_BOOLEAN:
                        {
                            const dbus_bool_t *b;
                            dbus_message_iter_get_fixed_array(&sub, &b, &datalen);
                            lua_createtable(L, datalen, 0);
                            for(int i = 0; i < datalen; i++)
                            {
                                lua_pushboolean(L, b[i]);
                                lua_rawseti(L, -2, i + 1);
                            }
                        }
                        break;
                    }
                }
                else if(array_type == DBUS_TYPE_DICT_ENTRY)
                {
                    DBusMessageIter subiter;
                    /* initialize a sub iterator */
                    dbus_message_iter_recurse(iter, &subiter);

                    /* get the keys and the values
                     * n is the number of entry in dict */
                    int n = a_dbus_message_iter(L, &subiter);

                    /* create a new table to store all the value */
                    lua_createtable(L, n, 0);
                    /* move the table before array elements */
                    lua_insert(L, - (n * 2) - 1);

                    for(int i = 0; i < n; i ++)
                        lua_rawset(L, - (n * 2) - 1 + i * 2);
                }
                else
                {
                    DBusMessageIter subiter;
                    /* prepare to dig into the array*/
                    dbus_message_iter_recurse(iter, &subiter);

                    /* now iterate over every element of the array */
                    int n = a_dbus_message_iter(L, &subiter);

                    /* create a new table to store all the value */
                    lua_createtable(L, n, 0);
                    /* move the table before array elements */
                    lua_insert(L, - n - 1);

                    for(int i = n; i > 0; i--)
                        lua_rawseti(L, - i - 1, i);
                }
            }
            nargs++;
            break;
          case DBUS_TYPE_BOOLEAN:
            {
                dbus_bool_t b;
                dbus_message_iter_get_basic(iter, &b);
                lua_pushboolean(L, b);
            }
            nargs++;
            break;
          case DBUS_TYPE_BYTE:
            {
                char c;
                dbus_message_iter_get_basic(iter, &c);
                lua_pushlstring(L, &c, 1);
            }
            nargs++;
            break;
#define DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(type, dbustype, pusher) \
          case dbustype: \
            { \
                type ui; \
                dbus_message_iter_get_basic(iter, &ui); \
                pusher(L, ui); \
            } \
            nargs++; \
            break;
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(int16_t, DBUS_TYPE_INT16, lua_pushinteger)
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(uint16_t, DBUS_TYPE_UINT16, lua_pushinteger)
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(int32_t, DBUS_TYPE_INT32, lua_pushinteger)
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(uint32_t, DBUS_TYPE_UINT32, lua_pushinteger)
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(int64_t, DBUS_TYPE_INT64, lua_pushinteger)
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(uint64_t, DBUS_TYPE_UINT64, lua_pushinteger)
          DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT(double, DBUS_TYPE_DOUBLE, lua_pushnumber)
#undef DBUS_MSG_HANDLE_TYPE_NUMBER_OR_INT
          case DBUS_TYPE_STRING:
            {
                char *s;
                dbus_message_iter_get_basic(iter, &s);
                lua_pushstring(L, s);
            }
            nargs++;
            break;
        }
    } while(dbus_message_iter_next(iter));

    return nargs;
}

static bool
a_dbus_convert_value(lua_State *L, int idx, DBusMessageIter *iter)
{
    /* i is the type name, i+1 the value */
    size_t len;
    const char *type = lua_tolstring(L, idx, &len);

    if(!type || len < 1)
        return false;

    switch(*type)
    {
      case DBUS_TYPE_ARRAY:
        {
            DBusMessageIter subiter;
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                             type + 1,
                                             &subiter);

            int arraylen = luaA_rawlen(L, idx + 1);

            if(arraylen % 2 != 0)
            {
                luaA_warn(L,
                          "your D-Bus signal handling method returned wrong number of arguments");
                return false;
            }

            /* Push the array */
            lua_pushvalue(L, idx + 1);

            for(int i = 1; i < arraylen; i += 2)
            {
                lua_rawgeti(L, -1, i);
                lua_rawgeti(L, -2, i + 1);
                if(!a_dbus_convert_value(L, -2, &subiter))
                    return false;
                lua_pop(L, 2);
            }

            /* Remove the array */
            lua_pop(L, 1);

            dbus_message_iter_close_container(iter, &subiter);
        }
        break;
      case DBUS_TYPE_BOOLEAN:
        {
            dbus_bool_t b = lua_toboolean(L, idx + 1);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &b);
        }
        break;
      case DBUS_TYPE_STRING:
        {
            const char *s = lua_tostring(L, idx + 1);
            if(!s || !dbus_validate_utf8(s, NULL)) {
                luaA_warn(L, "Your D-Bus signal handling method returned an invalid string");
                return false;
            }
            dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &s);
        }
        break;
#define DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(type, dbustype) \
      case dbustype: \
        { \
           type num = lua_tonumber(L, idx + 1); \
           dbus_message_iter_append_basic(iter, dbustype, &num); \
        } \
        break;
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint8_t, DBUS_TYPE_BYTE)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(int16_t, DBUS_TYPE_INT16)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint16_t, DBUS_TYPE_UINT16)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(int32_t, DBUS_TYPE_INT32)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint32_t, DBUS_TYPE_UINT32)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(int64_t, DBUS_TYPE_INT64)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(uint64_t, DBUS_TYPE_UINT64)
    DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER(double, DBUS_TYPE_DOUBLE)
#undef DBUS_MSG_RETURN_HANDLE_TYPE_NUMBER
    }

    return true;
}

/** Process a single request from D-Bus
 * \param dbus_connection  The connection to the D-Bus server.
 * \param msg The D-Bus message request being sent to the D-Bus connection.
 */
static void
a_dbus_process_request(DBusConnection *dbus_connection, DBusMessage *msg)
{
    const char *interface = dbus_message_get_interface(msg);
    lua_State *L = globalconf_get_lua_State();
    int old_top = lua_gettop(L);

    lua_createtable(L, 0, 5);

    switch(dbus_message_get_type(msg))
    {
      case DBUS_MESSAGE_TYPE_SIGNAL:
        lua_pushliteral(L, "signal");
        break;
      case DBUS_MESSAGE_TYPE_METHOD_CALL:
        lua_pushliteral(L, "method_call");
        break;
      case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        lua_pushliteral(L, "method_return");
        break;
      case DBUS_MESSAGE_TYPE_ERROR:
        lua_pushliteral(L, "error");
        break;
      default:
        lua_pushliteral(L, "unknown");
        break;
    }

    lua_setfield(L, -2, "type");

    lua_pushstring(L, interface);
    lua_setfield(L, -2, "interface");

    const char *s = dbus_message_get_path(msg);
    lua_pushstring(L, s);
    lua_setfield(L, -2, "path");

    s = dbus_message_get_member(msg);
    lua_pushstring(L, s);
    lua_setfield(L, -2, "member");

    s = dbus_message_get_sender(msg);
    if(s != NULL) {
        lua_pushstring(L, s);
        lua_setfield(L, -2, "sender");
    }

    if(dbus_connection == dbus_connection_system)
        lua_pushliteral(L, "system");
    else
        lua_pushliteral(L, "session");
    lua_setfield(L, -2, "bus");

    /* + 1 for the table above */
    DBusMessageIter iter;
    int nargs = 1;

    if(dbus_message_iter_init(msg, &iter))
        nargs += a_dbus_message_iter(L, &iter);

    if(dbus_message_get_no_reply(msg))
    {
        signal_t *sigfound = signal_array_getbyid(&dbus_signals,
                                                  a_strhash((const unsigned char *) NONULL(interface)));
        /* emit signals */
        if(sigfound)
            signal_object_emit(L, &dbus_signals, NONULL(interface), nargs);
    }
    else
    {
        signal_t *sig = signal_array_getbyid(&dbus_signals,
                                             a_strhash((const unsigned char *) NONULL(interface)));
        if(sig)
        {
            /* there can be only ONE handler to send reply */
            void *func = (void *) sig->sigfuncs.tab[0];

            int n = lua_gettop(L) - nargs;

            luaA_object_push(L, (void *) func);
            luaA_dofunction(L, nargs, LUA_MULTRET);

            n -= lua_gettop(L);

            DBusMessage *reply = dbus_message_new_method_return(msg);

            dbus_message_iter_init_append(reply, &iter);

            if(n % 2 != 0)
            {
                luaA_warn(L,
                          "your D-Bus signal handling method returned wrong number of arguments");
                /* Restore stack */
                lua_settop(L, old_top);
                return;
            }

            /* i is negative */
            for(int i = n; i < 0; i += 2)
            {
                if(!a_dbus_convert_value(L, i, &iter))
                {
                    luaA_warn(L, "your D-Bus signal handling method returned bad data");
                    /* Restore stack */
                    lua_settop(L, old_top);
                    return;
                }

                lua_remove(L, i);
                lua_remove(L, i + 1);
            }

            dbus_connection_send(dbus_connection, reply, NULL);
            dbus_message_unref(reply);
        }
    }
    /* Restore stack */
    lua_settop(L, old_top);
}

/** Attempt to process all the requests in the D-Bus connection.
 * \param dbus_connection The D-Bus connection to process from
 * \param source The D-Bus source
 */
static void
a_dbus_process_requests_on_bus(DBusConnection *dbus_connection, GSource **source)
{
    DBusMessage *msg;
    int nmsg = 0;

    while(true)
    {
        dbus_connection_read_write(dbus_connection, 0);

        if(!(msg = dbus_connection_pop_message(dbus_connection)))
            break;

        if(dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
        {
            a_dbus_cleanup_bus(dbus_connection, source);
            dbus_message_unref(msg);
            return;
        }
        else
            a_dbus_process_request(dbus_connection, msg);

        dbus_message_unref(msg);

        nmsg++;
    }

    if(nmsg)
        dbus_connection_flush(dbus_connection);
}

/** Foreword D-Bus process session requests on too the correct function.
 * \param data
 */
static gboolean
a_dbus_process_requests_session(gpointer data)
{
    a_dbus_process_requests_on_bus(dbus_connection_session, &session_source);
    return TRUE;
}

/** Foreword D-Bus process system requests on too the correct function.
 * \param data
 */
static gboolean
a_dbus_process_requests_system(gpointer data)
{
    a_dbus_process_requests_on_bus(dbus_connection_system, &system_source);
    return TRUE;
}

/** Attempt to request a D-Bus name
 * \param dbus_connection The application's connection to D-Bus
 * \param name The D-Bus connection name to be requested
 * \return true if the name is primary owner or the name is already
 * the owner, otherwise false.
 */
static bool
a_dbus_request_name(DBusConnection *dbus_connection, const char *name)
{
    DBusError err;

    if(!dbus_connection)
        return false;

    dbus_error_init(&err);

    int ret = dbus_bus_request_name(dbus_connection, name, 0, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to request D-Bus name: %s", err.message);
        dbus_error_free(&err);
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

/** Attempt to release the D-Bus name owner
 * \param dbus_connection The application's connection to the D-Bus
 * \param name The name to be released
 * \return True on success. False if the name is not
 * the owner, or does not exist.
 */
static bool
a_dbus_release_name(DBusConnection *dbus_connection, const char *name)
{
    DBusError err;

    if(!dbus_connection)
        return false;

    dbus_error_init(&err);

    int ret = dbus_bus_release_name(dbus_connection, name, &err);

    if(dbus_error_is_set(&err))
    {
        warn("failed to release D-Bus name: %s", err.message);
        dbus_error_free(&err);
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

/** Attempt to create a new connection to D-Bus
 * \param type The bus type to use when connecting to D-Bus
 * \param type_name The bus type name eg: "session" or "system"
 * \param cb Function callback to use when processing requests
 * \param source A new GSource that will be used for watching the dbus connection.
 * \return The requested D-Bus connection on success, NULL on failure.
 */
static DBusConnection *
a_dbus_connect(DBusBusType type, const char *type_name, GSourceFunc cb, GSource **source)
{
    int fd;
    DBusConnection *dbus_connection;
    DBusError err;

    dbus_error_init(&err);

    dbus_connection = dbus_bus_get(type, &err);
    if(dbus_error_is_set(&err))
    {
        warn("D-Bus session bus %s failed: %s", type_name, err.message);
        dbus_connection = NULL;
        dbus_error_free(&err);
    }
    else
    {
        dbus_connection_set_exit_on_disconnect(dbus_connection, false);
        if(dbus_connection_get_unix_fd(dbus_connection, &fd))
        {
            GIOChannel *channel = g_io_channel_unix_new(fd);
            *source = g_io_create_watch(channel, G_IO_IN);
            g_io_channel_unref(channel);
            g_source_set_callback(*source, cb, NULL, NULL);
            g_source_attach(*source, NULL);

            fcntl(fd, F_SETFD, FD_CLOEXEC);
        }
        else
        {
            warn("cannot get D-Bus connection file descriptor");
            a_dbus_cleanup_bus(dbus_connection, source);
        }
    }

    return dbus_connection;
}

/** Initialize the D-Bus session and system
 */
void
a_dbus_init(void)
{
    dbus_connection_session = a_dbus_connect(DBUS_BUS_SESSION, "session",
                                             a_dbus_process_requests_session, &session_source);
    dbus_connection_system = a_dbus_connect(DBUS_BUS_SYSTEM, "system",
                                            a_dbus_process_requests_system, &system_source);
}

/** Cleanup the D-Bus session and system
 */
void
a_dbus_cleanup(void)
{
    a_dbus_cleanup_bus(dbus_connection_session, &session_source);
    a_dbus_cleanup_bus(dbus_connection_system, &system_source);
}

/** Retrieve the D-Bus bus by it's name
 * \param name The name of the bus
 * \return The corresponding D-Bus connection
 */
static DBusConnection *
a_dbus_bus_getbyname(const char *name)
{
    if(A_STREQ(name, "system"))
        return dbus_connection_system;
    if(A_STREQ(name, "session"))
        return dbus_connection_session;
    return NULL;
}

/** Register a D-Bus name to receive message from.
 *
 * @param bus A string indicating if we are using system or session bus.
 * @param name A string with the name of the D-Bus name to register.
 * @return True if everything worked fine, false otherwise.
 * @function request_name
 */
static int
luaA_dbus_request_name(lua_State *L)
{
    const char *bus = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);
    DBusConnection *dbus_connection = a_dbus_bus_getbyname(bus);
    lua_pushboolean(L, a_dbus_request_name(dbus_connection, name));
    return 1;
}

/** Release a D-Bus name.
 *
 * @param bus A string indicating if we are using system or session bus.
 * @param name A string with the name of the D-Bus name to unregister.
 * @return True if everything worked fine, false otherwise.
 * @function release_name
 */
static int
luaA_dbus_release_name(lua_State *L)
{
    const char *bus = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);
    DBusConnection *dbus_connection = a_dbus_bus_getbyname(bus);
    lua_pushboolean(L, a_dbus_release_name(dbus_connection, name));
    return 1;
}

/** Add a match rule to match messages going through the message bus.
 *
 * @param bus A string indicating if we are using system or session bus.
 * @param name A string with the name of the match rule.
 * @function add_match
 */
static int
luaA_dbus_add_match(lua_State *L)
{
    const char *bus = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);
    DBusConnection *dbus_connection = a_dbus_bus_getbyname(bus);

    if(dbus_connection)
    {
        dbus_bus_add_match(dbus_connection, name, NULL);
        dbus_connection_flush(dbus_connection);
    }

    return 0;
}

/** Remove a previously added match rule "by value"
 * (the most recently-added identical rule gets removed).
 *
 * @param bus A string indicating if we are using system or session bus.
 * @param name A string with the name of the match rule.
 * @function remove_match
 */
static int
luaA_dbus_remove_match(lua_State *L)
{
    const char *bus = luaL_checkstring(L, 1);
    const char *name = luaL_checkstring(L, 2);
    DBusConnection *dbus_connection = a_dbus_bus_getbyname(bus);

    if(dbus_connection)
    {
        dbus_bus_remove_match(dbus_connection, name, NULL);
        dbus_connection_flush(dbus_connection);
    }

    return 0;
}

/** Add a signal receiver on the D-Bus.
 *
 * @param interface A string with the interface name.
 * @param func The function to call.
 * @function connect_signal
 */
static int
luaA_dbus_connect_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    signal_t *sig = signal_array_getbyid(&dbus_signals,
                                         a_strhash((const unsigned char *) name));
    if(sig)
        luaA_warn(L, "cannot add signal %s on D-Bus, already existing", name);
    else
        signal_connect(&dbus_signals, name, luaA_object_ref(L, 2));
    return 0;
}

/** Remove a signal receiver on the D-Bus.
 *
 * @param interface A string with the interface name.
 * @param func The function to call.
 * @function disconnect_signal
 */
static int
luaA_dbus_disconnect_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    const void *func = lua_topointer(L, 2);
    if (signal_disconnect(&dbus_signals, name, func))
        luaA_object_unref(L, func);
    return 0;
}

/** Emit a signal on the D-Bus.
 *
 * @param bus A string indicating if we are using system or session bus.
 * @param path A string with the dbus path.
 * @param interface A string with the dbus interface.
 * @param method A string with the dbus method name.
 * @param type_1st_arg type of 1st argument
 * @param value_1st_arg value of 1st argument
 * @param type_2nd_arg type of 2nd argument
 * @param value_2nd_arg value of 2nd argument
 * ... etc
 * @function emit_signal
 */
static int
luaA_dbus_emit_signal(lua_State *L)
{
    const char *bus_name = luaL_checkstring(L, 1);
    const char *path = luaL_checkstring(L, 2);
    const char *itface = luaL_checkstring(L, 3);
    const char *name = luaL_checkstring(L, 4);
    DBusConnection *dbus_connection = a_dbus_bus_getbyname(bus_name);
    DBusMessage* msg = dbus_message_new_signal(path, itface, name);
    if (msg == NULL) {
        luaA_warn(L, "your D-Bus signal emitting method error'd");
        return 0;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);
    int top = lua_gettop(L);
    int nargs = top - 4;

    if(nargs % 2 != 0)
    {
        luaA_warn(L, "your D-Bus signal emitting method has wrong number of arguments");
        dbus_message_unref(msg);
        lua_pushboolean(L, 0);
        return 1;
    }
    for(int i = 5; i < top; i += 2) {
        if(!a_dbus_convert_value(L, i, &iter))
        {
            luaA_warn(L, "your D-Bus signal emitting method has bad argument type");
            dbus_message_unref(msg);
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    dbus_connection_send(dbus_connection, msg, NULL);
    dbus_message_unref(msg);
    dbus_connection_flush(dbus_connection);
    lua_pushboolean(L, 1);
    return 1;
}

const struct luaL_Reg awesome_dbus_lib[] =
{
    { "request_name", luaA_dbus_request_name },
    { "release_name", luaA_dbus_release_name },
    { "add_match", luaA_dbus_add_match },
    { "remove_match", luaA_dbus_remove_match },
    { "connect_signal", luaA_dbus_connect_signal },
    { "disconnect_signal", luaA_dbus_disconnect_signal },
    { "emit_signal", luaA_dbus_emit_signal },
    { "__index", luaA_default_index },
    { "__newindex", luaA_default_newindex },
    { NULL, NULL }
};

#else /* WITH_DBUS */

/** Empty stub if dbus is not enabled */
void
a_dbus_init(void)
{
}

/** Empty stub if dbus is not enabled */
void
a_dbus_cleanup(void)
{
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
