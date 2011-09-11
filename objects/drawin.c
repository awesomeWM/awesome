/*
 * drawin.c - drawin functions
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright ©      2010 Uli Schlachter <psychon@znc.in>
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

#include "screen.h"
#include "drawin.h"
#include "objects/client.h"
#include "screen.h"
#include "xwindow.h"
#include "luaa.h"
#include "ewmh.h"
#include "systray.h"
#include "common/xcursor.h"
#include "common/xutil.h"

#include <cairo-xcb.h>

LUA_OBJECT_FUNCS(drawin_class, drawin_t, drawin)

/** Kick out systray windows.
 */
static void
drawin_systray_kickout(drawin_t *w)
{
    if(globalconf.systray.parent == w)
    {
        /* Who! Check that we're not deleting a drawin with a systray, because it
         * may be its parent. If so, we reparent to root before, otherwise it will
         * hurt very much. */
        systray_cleanup();
        xcb_reparent_window(globalconf.connection,
                            globalconf.systray.window,
                            globalconf.screen->root,
                            -512, -512);

        globalconf.systray.parent = NULL;
    }
}

static void
drawin_wipe(drawin_t *w)
{
    /* The drawin must already be unmapped, else it
     * couldn't be garbage collected -> no unmap needed */
    p_delete(&w->cursor);
    if(w->surface)
    {
        /* Make sure that cairo knows that this surface can't be unused anymore.
         * This is needed since lua could still have a reference to it. */
        cairo_surface_finish(w->surface);
        cairo_surface_destroy(w->surface);
        w->surface = NULL;
    }
    if(w->window)
    {
        /* Activate BMA */
        client_ignore_enterleave_events();
        /* Make sure we don't accidentally kill the systray window */
        drawin_systray_kickout(w);
        xcb_destroy_window(globalconf.connection, w->window);
        /* Deactivate BMA */
        client_restore_enterleave_events();
        w->window = XCB_NONE;
    }
    if(w->pixmap)
    {
        xcb_free_pixmap(globalconf.connection, w->pixmap);
        w->pixmap = XCB_NONE;
    }
}

void
drawin_unref_simplified(drawin_t **item)
{
    luaA_object_unref(globalconf.L, *item);
}

static void
drawin_update_drawing(drawin_t *w)
{
    /* If this drawin isn't visible, we don't need an up-to-date cairo surface
     * for it. (drawin_map() will later make sure we are called again) */
    if(!w->visible)
        return;
    /* Clean up old stuff */
    if(w->surface)
    {
        /* In case lua still got a reference to the surface, it still won't be
         * able to do anything with it because we finish it. */
        cairo_surface_finish(w->surface);
        cairo_surface_destroy(w->surface);
    }
    if(w->pixmap)
        xcb_free_pixmap(globalconf.connection, w->pixmap);

    /* Create a pixmap */
    xcb_screen_t *s = globalconf.screen;
    w->pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, globalconf.default_depth, w->pixmap, s->root,
                      w->geometry.width, w->geometry.height);
    /* and create a surface for that pixmap */
    w->surface = cairo_xcb_surface_create(globalconf.connection,
                                          w->pixmap, globalconf.visual,
                                          w->geometry.width, w->geometry.height);
    /* Make sure the pixmap doesn't contain garbage by filling it with black */
    cairo_t *cr = cairo_create(w->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_destroy(cr);
}

/** Initialize a drawin.
 * \param w The drawin to initialize.
 */
static void
drawin_init(drawin_t *w)
{
    xcb_screen_t *s = globalconf.screen;

    w->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.default_depth, w->window, s->root,
                      w->geometry.x, w->geometry.y,
                      w->geometry.width, w->geometry.height,
                      w->border_width, XCB_COPY_FROM_PARENT, globalconf.visual->visual_id,
                      XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_BIT_GRAVITY
                      | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP
                      | XCB_CW_CURSOR,
                      (const uint32_t [])
                      {
                          w->bg_color.pixel,
                          w->border_color.pixel,
                          XCB_GRAVITY_NORTH_WEST,
                          1,
                          XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                          | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
                          | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                          | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE
                          | XCB_EVENT_MASK_PROPERTY_CHANGE,
                          globalconf.default_cmap,
                          xcursor_new(globalconf.connection, xcursor_font_fromstr(w->cursor))
                      });

    /* Set the right properties */
    ewmh_update_window_type(w->window, window_translate_type(w->type));
    ewmh_update_strut(w->window, &w->strut);
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param w The drawin to refresh.
 */
static inline void
drawin_refresh_pixmap(drawin_t *w)
{
    drawin_refresh_pixmap_partial(w, 0, 0, w->geometry.width, w->geometry.height);
}

/** Move and/or resize a drawin
 * \param L The Lua VM state.
 * \param udx The index of the drawin.
 * \param geometry The new geometry.
 */
static void
drawin_moveresize(lua_State *L, int udx, area_t geometry)
{
    drawin_t *w = luaA_checkudata(L, udx, &drawin_class);
    int number_of_vals = 0;
    uint32_t moveresize_win_vals[4], mask_vals = 0;

    if(w->geometry.x != geometry.x)
    {
        w->geometry.x = moveresize_win_vals[number_of_vals++] = geometry.x;
        mask_vals |= XCB_CONFIG_WINDOW_X;
    }

    if(w->geometry.y != geometry.y)
    {
        w->geometry.y = moveresize_win_vals[number_of_vals++] = geometry.y;
        mask_vals |= XCB_CONFIG_WINDOW_Y;
    }

    if(geometry.width > 0 && w->geometry.width != geometry.width)
    {
        w->geometry.width = moveresize_win_vals[number_of_vals++] = geometry.width;
        mask_vals |= XCB_CONFIG_WINDOW_WIDTH;
    }

    if(geometry.height > 0 && w->geometry.height != geometry.height)
    {
        w->geometry.height = moveresize_win_vals[number_of_vals++] = geometry.height;
        mask_vals |= XCB_CONFIG_WINDOW_HEIGHT;
    }

    if(mask_vals & (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT))
        drawin_update_drawing(w);

    /* Activate BMA */
    client_ignore_enterleave_events();

    if(mask_vals)
        xcb_configure_window(globalconf.connection, w->window, mask_vals, moveresize_win_vals);

    /* Deactivate BMA */
    client_restore_enterleave_events();

    if(mask_vals & XCB_CONFIG_WINDOW_X)
        luaA_object_emit_signal(L, udx, "property::x", 0);
    if(mask_vals & XCB_CONFIG_WINDOW_Y)
        luaA_object_emit_signal(L, udx, "property::y", 0);
    if(mask_vals & XCB_CONFIG_WINDOW_WIDTH)
        luaA_object_emit_signal(L, udx, "property::width", 0);
    if(mask_vals & XCB_CONFIG_WINDOW_HEIGHT)
        luaA_object_emit_signal(L, udx, "property::height", 0);
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param drawin The drawin to refresh.
 * \param x The copy starting point x component.
 * \param y The copy starting point y component.
 * \param w The copy width from the x component.
 * \param h The copy height from the y component.
 */
void
drawin_refresh_pixmap_partial(drawin_t *drawin,
                              int16_t x, int16_t y,
                              uint16_t w, uint16_t h)
{
    /* Make cairo do all pending drawing */
    cairo_surface_flush(drawin->surface);
    xcb_copy_area(globalconf.connection, drawin->pixmap,
                  drawin->window, globalconf.gc, x, y, x, y,
                  w, h);
}

static void
drawin_map(drawin_t *drawin)
{
    /* Activate BMA */
    client_ignore_enterleave_events();
    /* Map the drawin */
    xcb_map_window(globalconf.connection, drawin->window);
    drawin_update_drawing(drawin);
    /* Deactivate BMA */
    client_restore_enterleave_events();
    /* Stack this drawin correctly */
    stack_windows();
    /* Add it to the list of visible drawins */
    drawin_array_append(&globalconf.drawins, drawin);
}

static void
drawin_unmap(drawin_t *drawin)
{
    xcb_unmap_window(globalconf.connection, drawin->window);
    foreach(item, globalconf.drawins)
        if(*item == drawin)
        {
            drawin_array_remove(&globalconf.drawins, item);
            break;
        }
}

/** Get a drawin by its window.
 * \param win The window id.
 * \return A drawin if found, NULL otherwise.
 */
drawin_t *
drawin_getbywin(xcb_window_t win)
{
    foreach(w, globalconf.drawins)
        if((*w)->window == win)
            return *w;
    return NULL;
}

/** Set a drawin visible or not.
 * \param L The Lua VM state.
 * \param udx The drawin.
 * \param v The visible value.
 */
static void
drawin_set_visible(lua_State *L, int udx, bool v)
{
    drawin_t *drawin = luaA_checkudata(L, udx, &drawin_class);
    if(v != drawin->visible)
    {
        drawin->visible = v;

        if(drawin->visible)
        {
            drawin_map(drawin);
            /* duplicate drawin */
            lua_pushvalue(L, udx);
            /* ref it */
            luaA_object_ref_class(globalconf.L, -1, &drawin_class);
        }
        else
        {
            /* Active BMA */
            client_ignore_enterleave_events();
            /* Unmap window */
            drawin_unmap(drawin);
            /* Active BMA */
            client_restore_enterleave_events();
            /* unref it */
            luaA_object_unref(globalconf.L, drawin);
        }

        luaA_object_emit_signal(L, udx, "property::visible", 0);
        if(strut_has_value(&drawin->strut))
        {
            screen_t *screen =
                screen_getbycoord(drawin->geometry.x, drawin->geometry.y);
            screen_emit_signal(globalconf.L, screen, "property::workarea", 0);
        }
    }
}

/** Create a new drawin.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_new(lua_State *L)
{
    luaA_class_new(L, &drawin_class);

    drawin_t *w = luaA_checkudata(L, -1, &drawin_class);

    w->visible = false;

    if(!w->opacity)
        w->opacity = -1;

    if(!w->cursor)
        w->cursor = a_strdup("left_ptr");

    if(!w->geometry.width)
        w->geometry.width = 1;

    if(!w->geometry.height)
        w->geometry.height = 1;

    if(w->type == 0)
        w->type = _NET_WM_WINDOW_TYPE_NORMAL;

    drawin_init(w);

    return 1;
}

/* Set or get the drawin geometry.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam An optional table with drawin geometry.
 * \lreturn The drawin geometry.
 */
static int
luaA_drawin_geometry(lua_State *L)
{
    drawin_t *drawin = luaA_checkudata(L, 1, &drawin_class);

    if(lua_gettop(L) == 2)
    {
        area_t wingeom;

        luaA_checktable(L, 2);
        wingeom.x = luaA_getopt_number(L, 2, "x", drawin->geometry.x);
        wingeom.y = luaA_getopt_number(L, 2, "y", drawin->geometry.y);
        wingeom.width = luaA_getopt_number(L, 2, "width", drawin->geometry.width);
        wingeom.height = luaA_getopt_number(L, 2, "height", drawin->geometry.height);

        if(wingeom.width > 0 && wingeom.height > 0)
            drawin_moveresize(L, 1, wingeom);
    }

    return luaA_pusharea(L, drawin->geometry);
}

/** Set the drawin background color.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_bg_color(lua_State *L, drawin_t *drawin)
{
    size_t len;
    const char *color_name = luaL_checklstring(L, -1, &len);

    if(color_name &&
       color_init_reply(color_init_unchecked(&drawin->bg_color, color_name, len)))
    {
        xcb_change_window_attributes(globalconf.connection, drawin->window,
                XCB_CW_BACK_PIXEL, &drawin->bg_color.pixel);
        luaA_object_emit_signal(L, -3, "property::bg_color", 0);
    }

    return 0;
}


LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, ontop, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, cursor, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, visible, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, bg_color, luaA_pushcolor)

static int
luaA_drawin_set_x(lua_State *L, drawin_t *drawin)
{
    drawin_moveresize(L, -3, (area_t) { .x = luaL_checknumber(L, -1),
                                        .y = drawin->geometry.y,
                                        .width = drawin->geometry.width,
                                        .height = drawin->geometry.height });
    return 0;
}

static int
luaA_drawin_get_x(lua_State *L, drawin_t *drawin)
{
    lua_pushnumber(L, drawin->geometry.x);
    return 1;
}

static int
luaA_drawin_set_y(lua_State *L, drawin_t *drawin)
{
    drawin_moveresize(L, -3, (area_t) { .x = drawin->geometry.x,
                                        .y = luaL_checknumber(L, -1),
                                        .width = drawin->geometry.width,
                                        .height = drawin->geometry.height });
    return 0;
}

static int
luaA_drawin_get_y(lua_State *L, drawin_t *drawin)
{
    lua_pushnumber(L, drawin->geometry.y);
    return 1;
}

static int
luaA_drawin_set_width(lua_State *L, drawin_t *drawin)
{
    int width = luaL_checknumber(L, -1);
    if(width <= 0)
        luaL_error(L, "invalid width");
    drawin_moveresize(L, -3, (area_t) { .x = drawin->geometry.x,
                                        .y = drawin->geometry.y,
                                        .width = width,
                                        .height = drawin->geometry.height });
    return 0;
}

static int
luaA_drawin_get_width(lua_State *L, drawin_t *drawin)
{
    lua_pushnumber(L, drawin->geometry.width);
    return 1;
}

static int
luaA_drawin_set_height(lua_State *L, drawin_t *drawin)
{
    int height = luaL_checknumber(L, -1);
    if(height <= 0)
        luaL_error(L, "invalid height");
    drawin_moveresize(L, -3, (area_t) { .x = drawin->geometry.x,
                                       .y = drawin->geometry.y,
                                       .width = drawin->geometry.width,
                                       .height = height });
    return 0;
}

static int
luaA_drawin_get_height(lua_State *L, drawin_t *drawin)
{
    lua_pushnumber(L, drawin->geometry.height);
    return 1;
}

/** Set the drawin on top status.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_ontop(lua_State *L, drawin_t *drawin)
{
    bool b = luaA_checkboolean(L, -1);
    if(b != drawin->ontop)
    {
        drawin->ontop = b;
        stack_windows();
        luaA_object_emit_signal(L, -3, "property::ontop", 0);
    }
    return 0;
}

/** Set the drawin cursor.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_cursor(lua_State *L, drawin_t *drawin)
{
    const char *buf = luaL_checkstring(L, -1);
    if(buf)
    {
        uint16_t cursor_font = xcursor_font_fromstr(buf);
        if(cursor_font)
        {
            xcb_cursor_t cursor = xcursor_new(globalconf.connection, cursor_font);
            p_delete(&drawin->cursor);
            drawin->cursor = a_strdup(buf);
            xwindow_set_cursor(drawin->window, cursor);
            luaA_object_emit_signal(L, -3, "property::cursor", 0);
        }
    }
    return 0;
}

/** Set the drawin visibility.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_visible(lua_State *L, drawin_t *drawin)
{
    drawin_set_visible(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

/** Get a drawin's surface
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_surface(lua_State *L, drawin_t *drawin)
{
    return oocairo_surface_push(L, drawin->surface);
}

/** Refresh a drawin's content. This has to be called whenever some drawing to
 * the drawin's surface has been done and should become visible.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_refresh(lua_State *L)
{
    drawin_t *drawin = luaA_checkudata(L, 1, &drawin_class);
    drawin_refresh_pixmap(drawin);

    return 0;
}

void
drawin_class_setup(lua_State *L)
{
    static const struct luaL_reg drawin_methods[] =
    {
        LUA_CLASS_METHODS(drawin)
        { "__call", luaA_drawin_new },
        { NULL, NULL }
    };

    static const struct luaL_reg drawin_meta[] =
    {
        LUA_OBJECT_META(drawin)
        LUA_CLASS_META
        { "geometry", luaA_drawin_geometry },
        { "refresh", luaA_drawin_refresh },
        { NULL, NULL },
    };

    luaA_class_setup(L, &drawin_class, "drawin", &window_class,
                     (lua_class_allocator_t) drawin_new,
                     (lua_class_collector_t) drawin_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     drawin_methods, drawin_meta);
    luaA_class_add_property(&drawin_class, "surface",
                            NULL,
                            (lua_class_propfunc_t) luaA_drawin_get_surface,
                            NULL);
    luaA_class_add_property(&drawin_class, "visible",
                            (lua_class_propfunc_t) luaA_drawin_set_visible,
                            (lua_class_propfunc_t) luaA_drawin_get_visible,
                            (lua_class_propfunc_t) luaA_drawin_set_visible);
    luaA_class_add_property(&drawin_class, "ontop",
                            (lua_class_propfunc_t) luaA_drawin_set_ontop,
                            (lua_class_propfunc_t) luaA_drawin_get_ontop,
                            (lua_class_propfunc_t) luaA_drawin_set_ontop);
    luaA_class_add_property(&drawin_class, "cursor",
                            (lua_class_propfunc_t) luaA_drawin_set_cursor,
                            (lua_class_propfunc_t) luaA_drawin_get_cursor,
                            (lua_class_propfunc_t) luaA_drawin_set_cursor);
    luaA_class_add_property(&drawin_class, "x",
                            (lua_class_propfunc_t) luaA_drawin_set_x,
                            (lua_class_propfunc_t) luaA_drawin_get_x,
                            (lua_class_propfunc_t) luaA_drawin_set_x);
    luaA_class_add_property(&drawin_class, "y",
                            (lua_class_propfunc_t) luaA_drawin_set_y,
                            (lua_class_propfunc_t) luaA_drawin_get_y,
                            (lua_class_propfunc_t) luaA_drawin_set_y);
    luaA_class_add_property(&drawin_class, "width",
                            (lua_class_propfunc_t) luaA_drawin_set_width,
                            (lua_class_propfunc_t) luaA_drawin_get_width,
                            (lua_class_propfunc_t) luaA_drawin_set_width);
    luaA_class_add_property(&drawin_class, "height",
                            (lua_class_propfunc_t) luaA_drawin_set_height,
                            (lua_class_propfunc_t) luaA_drawin_get_height,
                            (lua_class_propfunc_t) luaA_drawin_set_height);
    luaA_class_add_property(&window_class, "bg_color",
                            (lua_class_propfunc_t) luaA_drawin_set_bg_color,
                            (lua_class_propfunc_t) luaA_drawin_get_bg_color,
                            (lua_class_propfunc_t) luaA_drawin_set_bg_color);
    luaA_class_add_property(&drawin_class, "type",
                            (lua_class_propfunc_t) luaA_window_set_type,
                            (lua_class_propfunc_t) luaA_window_get_type,
                            (lua_class_propfunc_t) luaA_window_set_type);

    signal_add(&drawin_class.signals, "mouse::enter");
    signal_add(&drawin_class.signals, "mouse::leave");
    signal_add(&drawin_class.signals, "property::bg_color");
    signal_add(&drawin_class.signals, "property::border_width");
    signal_add(&drawin_class.signals, "property::cursor");
    signal_add(&drawin_class.signals, "property::height");
    signal_add(&drawin_class.signals, "property::ontop");
    signal_add(&drawin_class.signals, "property::visible");
    signal_add(&drawin_class.signals, "property::widgets");
    signal_add(&drawin_class.signals, "property::width");
    signal_add(&drawin_class.signals, "property::x");
    signal_add(&drawin_class.signals, "property::y");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
