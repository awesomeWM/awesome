/*
 * drawable.c - drawable functions
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2010-2012 Uli Schlachter <psychon@znc.in>
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

#include "drawable.h"

#include <cairo-xcb.h>

static lua_class_t drawable_class;

LUA_OBJECT_FUNCS(drawable_class, drawable_t, drawable)

drawable_t *
drawable_allocator(lua_State *L, drawable_refresh_callback *callback, void *data)
{
    drawable_t *d = drawable_new(L);
    d->refresh_callback = callback;
    d->refresh_data = data;
    d->refreshed = false;
    d->surface = NULL;
    d->pixmap = XCB_NONE;
    return d;
}

static void
drawable_unset_surface(drawable_t *d)
{
    cairo_surface_finish(d->surface);
    cairo_surface_destroy(d->surface);
    if (d->pixmap)
        xcb_free_pixmap(globalconf.connection, d->pixmap);
    d->refreshed = false;
    d->surface = NULL;
    d->pixmap = XCB_NONE;
}

static void
drawable_wipe(drawable_t *d)
{
    drawable_unset_surface(d);
}

void
drawable_set_geometry(drawable_t *d, int didx, area_t geom)
{
    area_t old = d->geometry;
    d->geometry = geom;

    bool size_changed = (old.width != geom.width) || (old.height != geom.height);
    if (size_changed)
        drawable_unset_surface(d);
    if (size_changed && geom.width > 0 && geom.height > 0)
    {
        d->pixmap = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, globalconf.default_depth, d->pixmap,
                          globalconf.screen->root, geom.width, geom.height);
        d->surface = cairo_xcb_surface_create(globalconf.connection,
                                              d->pixmap, globalconf.visual,
                                              geom.width, geom.height);
        luaA_object_emit_signal(globalconf.L, didx, "property::surface", 0);
    }

    if (old.x != geom.x)
        luaA_object_emit_signal(globalconf.L, didx, "property::x", 0);
    if (old.y != geom.y)
        luaA_object_emit_signal(globalconf.L, didx, "property::y", 0);
    if (old.width != geom.width)
        luaA_object_emit_signal(globalconf.L, didx, "property::width", 0);
    if (old.height != geom.height)
        luaA_object_emit_signal(globalconf.L, didx, "property::height", 0);
}

/** Get a drawable's surface
 * \param L The Lua VM state.
 * \param drawable The drawable object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawable_get_surface(lua_State *L, drawable_t *drawable)
{
    /* Lua gets its own reference which it will have to destroy */
    lua_pushlightuserdata(L, cairo_surface_reference(drawable->surface));
    return 1;
}

/** Refresh a drawable's content. This has to be called whenever some drawing to
 * the drawable's surface has been done and should become visible.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawable_refresh(lua_State *L)
{
    drawable_t *drawable = luaA_checkudata(L, 1, &drawable_class);
    drawable->refreshed = true;
    (*drawable->refresh_callback)(drawable->refresh_data);

    return 0;
}

/** Return drawable geometry.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lreturn A table with drawable coordinates.
 */
static int
luaA_drawable_geometry(lua_State *L)
{
    drawable_t *d = luaA_checkudata(L, 1, &drawable_class);
    return luaA_pusharea(L, d->geometry);
}

void
drawable_class_setup(lua_State *L)
{
    static const struct luaL_Reg drawable_methods[] =
    {
        LUA_CLASS_METHODS(drawable)
        { NULL, NULL }
    };

    static const struct luaL_Reg drawable_meta[] =
    {
        LUA_OBJECT_META(drawable)
        LUA_CLASS_META
        { "refresh", luaA_drawable_refresh },
        { "geometry", luaA_drawable_geometry },
        { NULL, NULL },
    };

    luaA_class_setup(L, &drawable_class, "drawable", NULL,
                     (lua_class_allocator_t) drawable_new,
                     (lua_class_collector_t) drawable_wipe, NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     drawable_methods, drawable_meta);
    luaA_class_add_property(&drawable_class, "surface",
                            NULL,
                            (lua_class_propfunc_t) luaA_drawable_get_surface,
                            NULL);

    signal_add(&drawable_class.signals, "button::press");
    signal_add(&drawable_class.signals, "button::release");
    signal_add(&drawable_class.signals, "mouse::enter");
    signal_add(&drawable_class.signals, "mouse::leave");
    signal_add(&drawable_class.signals, "mouse::move");
    signal_add(&drawable_class.signals, "property::height");
    signal_add(&drawable_class.signals, "property::width");
    signal_add(&drawable_class.signals, "property::x");
    signal_add(&drawable_class.signals, "property::y");
    signal_add(&drawable_class.signals, "property::surface");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
