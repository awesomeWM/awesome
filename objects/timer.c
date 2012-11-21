/*
 * timer.c - Timer signals management
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

#include "globalconf.h"
#include "luaa.h"
#include "timer.h"
#include "common/luaobject.h"

typedef struct
{
    LUA_OBJECT_HEADER
    bool started;
    guint source_id;
    double timeout;
} atimer_t;

static lua_class_t timer_class;
LUA_OBJECT_FUNCS(timer_class, atimer_t, timer)

static gboolean
timer_emit_signal(gpointer data)
{
    luaA_object_push(globalconf.L, data);
    luaA_object_emit_signal(globalconf.L, -1, "timeout", 0);
    lua_pop(globalconf.L, 1);
    return TRUE;
}

static int
luaA_timer_new(lua_State *L)
{
    luaA_class_new(L, &timer_class);
    return 1;
}

static int
luaA_timer_set_timeout(lua_State *L, atimer_t *timer)
{
    double timeout = luaL_checknumber(L, -1);
    timer->timeout = timeout;
    luaA_object_emit_signal(L, -3, "property::timeout", 0);
    return 0;
}

static int
luaA_timer_get_timeout(lua_State *L, atimer_t *timer)
{
    lua_pushnumber(L, timer->timeout);
    return 1;
}

static int
luaA_timer_start(lua_State *L)
{
    atimer_t *timer = luaA_checkudata(L, 1, &timer_class);
    if(timer->started)
        luaA_warn(L, "timer already started");
    else
    {
        luaA_object_ref(L, 1);
        timer->started = true;
        timer->source_id = g_timeout_add(timer->timeout * 1000, timer_emit_signal, timer);
    }
    return 0;
}

static int
luaA_timer_stop(lua_State *L)
{
    atimer_t *timer = luaA_checkudata(L, 1, &timer_class);
    if(timer->started)
    {
        g_source_remove(timer->source_id);
        luaA_object_unref(L, timer);
        timer->started = false;
    }
    else
        luaA_warn(L, "timer not started");
    return 0;
}

static int
luaA_timer_again(lua_State *L)
{
    atimer_t *timer = luaA_checkudata(L, 1, &timer_class);

    if (timer->started)
        g_source_remove(timer->source_id);
    else
        luaA_object_ref(L, 1);
    timer->started = true;
    timer->source_id = g_timeout_add(timer->timeout * 1000, timer_emit_signal, timer);

    return 0;
}

LUA_OBJECT_EXPORT_PROPERTY(timer, atimer_t, started, lua_pushboolean)

void
timer_class_setup(lua_State *L)
{
    static const struct luaL_Reg timer_methods[] =
    {
        LUA_CLASS_METHODS(timer)
        { "__call", luaA_timer_new },
        { NULL, NULL }
    };

    static const struct luaL_Reg timer_meta[] =
    {
        LUA_OBJECT_META(timer)
            LUA_CLASS_META
            { "start", luaA_timer_start },
            { "stop", luaA_timer_stop },
            { "again", luaA_timer_again },
            { NULL, NULL },
    };

    luaA_class_setup(L, &timer_class, "timer", NULL,
                     (lua_class_allocator_t) timer_new, NULL, NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     timer_methods, timer_meta);
    luaA_class_add_property(&timer_class, "timeout",
                            (lua_class_propfunc_t) luaA_timer_set_timeout,
                            (lua_class_propfunc_t) luaA_timer_get_timeout,
                            (lua_class_propfunc_t) luaA_timer_set_timeout);
    luaA_class_add_property(&timer_class, "started",
                            NULL,
                            (lua_class_propfunc_t) luaA_timer_get_started,
                            NULL);

    signal_add(&timer_class.signals, "property::timeout");
    signal_add(&timer_class.signals, "timeout");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
