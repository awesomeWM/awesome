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

/** awesome drawin API
 *
 * Furthermore to the classes described here, one can also use signals as
 * described in @{signals} and X properties as described in @{xproperties}.
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @classmod drawin
 */

#include "drawin.h"
#include "common/atoms.h"
#include "common/xcursor.h"
#include "common/xutil.h"
#include "objects/client.h"
#include "objects/screen.h"
#include "systray.h"

#include "x11/event.h"
#include "x11/ewmh.h"
#include "x11/xwindow.h"

#include "math.h"

#include <cairo-xcb.h>
#include <xcb/shape.h>

/** Drawin object.
 *
 * @field border_width Border width.
 * @field border_color Border color.
 * @field ontop On top of other windows.
 * @field cursor The mouse cursor.
 * @field visible Visibility.
 * @field opacity The opacity of the drawin, between 0 and 1.
 * @field type The window type (desktop, normal, dock, …).
 * @field x The x coordinates.
 * @field y The y coordinates.
 * @field width The width of the drawin.
 * @field height The height of the drawin.
 * @field drawable The drawin's drawable.
 * @field window The X window id.
 * @field shape_bounding The drawin's bounding shape as a (native) cairo surface.
 * @field shape_clip The drawin's clip shape as a (native) cairo surface.
 * @field shape_input The drawin's input shape as a (native) cairo surface.
 * @table drawin
 */

/**
 * @signal property::geometry
 */

/**
 * @signal property::shape_bounding
 */

/**
 * @signal property::shape_clip
 */

/**
 * @signal property::shape_input
 */

/**
 * @signal property::border_width
 */

/**
 * @signal property::cursor
 */

/**
 * @signal property::height
 */

/**
 * @signal property::ontop
 */

/**
 * @signal property::visible
 */

/**
 * @signal property::width
 */

/**
 * @signal property::x
 */

/**
 * @signal property::y
 */

/** Get or set mouse buttons bindings to a drawin.
 *
 * @param buttons_table A table of buttons objects, or nothing.
 * @function buttons
 */

/** Get or set drawin struts.
 *
 * @param strut A table with new strut, or nothing
 * @return The drawin strut in a table.
 * @function struts
 */

/** Get the number of instances.
 *
 * @return The number of drawin objects alive.
 * @function instances
 */

/** Set a __index metamethod for all drawin instances.
 * @tparam function cb The meta-method
 * @function set_index_miss_handler
 */

/** Set a __newindex metamethod for all drawin instances.
 * @tparam function cb The meta-method
 * @function set_newindex_miss_handler
 */

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
        xcb_reparent_window(globalconf.connection,
                            globalconf.systray.window,
                            globalconf.screen->root,
                            -512, -512);

        globalconf.systray.parent = NULL;
    }
}

void
luaA_drawin_systray_kickout(lua_State *L)
{
    drawin_systray_kickout(luaA_checkudata(L, 1, &drawin_class));
}

static void
drawin_wipe(drawin_t *w)
{
    /* The drawin must already be unmapped, else it
     * couldn't be garbage collected -> no unmap needed */
    p_delete(&w->cursor);
    if(w->window)
    {
        /* Make sure we don't accidentally kill the systray window */
        drawin_systray_kickout(w);
        xcb_destroy_window(globalconf.connection, w->window);
        w->window = XCB_NONE;
    }
    /* No unref needed because we are being garbage collected */
    w->drawable = NULL;
}

static void
drawin_update_drawing(lua_State *L, int widx)
{
    drawin_t *w = luaA_checkudata(L, widx, &drawin_class);
    luaA_object_push_item(L, widx, w->drawable);
    drawable_set_geometry(L, -1, w->geometry);
    lua_pop(L, 1);
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param w The drawin to refresh.
 */
static inline void
drawin_refresh_pixmap(drawin_t *w)
{
    drawin_refresh_pixmap_partial(w, 0, 0, w->geometry.width, w->geometry.height);
}

static void
drawin_apply_moveresize(drawin_t *w)
{
    if (!w->geometry_dirty)
        return;

    w->geometry_dirty = false;
    client_ignore_enterleave_events();
    xcb_configure_window(globalconf.connection, w->window,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                         | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         (const uint32_t [])
                         {
                             w->geometry.x,
                             w->geometry.y,
                             w->geometry.width,
                             w->geometry.height
                         });
    client_restore_enterleave_events();
}

void
drawin_refresh(void)
{
    foreach(item, globalconf.drawins)
    {
        drawin_apply_moveresize(*item);
        window_border_refresh((window_t *) *item);
    }
}

/** Get all drawins into a table.
 * @treturn table A table with drawins.
 * @function get
 */
static int
luaA_drawin_get(lua_State *L)
{
    int i = 1;

    lua_newtable(L);

    foreach(d, globalconf.drawins) {
        luaA_object_push(L, *d);
        lua_rawseti(L, -2, i++);
    }

    return 1;
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
    area_t old_geometry = w->geometry;

    w->geometry = geometry;
    if(w->geometry.width <= 0)
        w->geometry.width = old_geometry.width;
    if(w->geometry.height <= 0)
        w->geometry.height = old_geometry.height;

    w->geometry_dirty = true;
    drawin_update_drawing(L, udx);

    if (!AREA_EQUAL(old_geometry, w->geometry))
        luaA_object_emit_signal(L, udx, "property::geometry", 0);
    if (old_geometry.x != w->geometry.x)
        luaA_object_emit_signal(L, udx, "property::x", 0);
    if (old_geometry.y != w->geometry.y)
        luaA_object_emit_signal(L, udx, "property::y", 0);
    if (old_geometry.width != w->geometry.width)
        luaA_object_emit_signal(L, udx, "property::width", 0);
    if (old_geometry.height != w->geometry.height)
        luaA_object_emit_signal(L, udx, "property::height", 0);

    screen_t *old_screen = screen_getbycoord(old_geometry.x, old_geometry.y);
    screen_t *new_screen = screen_getbycoord(w->geometry.x, w->geometry.y);
    if (old_screen != new_screen && strut_has_value(&w->strut))
    {
        screen_update_workarea(old_screen);
        screen_update_workarea(new_screen);
    }
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
    if (!drawin->drawable || !drawin->drawable->pixmap || !drawin->drawable->refreshed)
        return;

    /* Make sure it really has the size it should have */
    drawin_apply_moveresize(drawin);

    /* Make cairo do all pending drawing */
    cairo_surface_flush(drawin->drawable->surface);
    xcb_copy_area(globalconf.connection, drawin->drawable->pixmap,
                  drawin->window, globalconf.gc, x, y, x, y,
                  w, h);
}

static void
drawin_map(lua_State *L, int widx)
{
    drawin_t *drawin = luaA_checkudata(L, widx, &drawin_class);
    /* Apply any pending changes */
    drawin_apply_moveresize(drawin);
    /* Activate BMA */
    client_ignore_enterleave_events();
    /* Map the drawin */
    xcb_map_window(globalconf.connection, drawin->window);
    /* Deactivate BMA */
    client_restore_enterleave_events();
    /* Stack this drawin correctly */
    stack_windows();
    /* Add it to the list of visible drawins */
    drawin_array_append(&globalconf.drawins, drawin);
    /* Make sure it has a surface */
    if(drawin->drawable->surface == NULL)
        drawin_update_drawing(L, widx);
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
            drawin_map(L, udx);
            /* duplicate drawin */
            lua_pushvalue(L, udx);
            /* ref it */
            luaA_object_ref_class(L, -1, &drawin_class);
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
            luaA_object_unref(L, drawin);
        }

        luaA_object_emit_signal(L, udx, "property::visible", 0);
        if(strut_has_value(&drawin->strut))
        {
            screen_update_workarea(
                    screen_getbycoord(drawin->geometry.x, drawin->geometry.y));
        }
    }
}

static drawin_t *
drawin_allocator(lua_State *L)
{
    xcb_screen_t *s = globalconf.screen;
    drawin_t *w = drawin_new(L);

    w->visible = false;

    w->opacity = -1;
    w->cursor = a_strdup("left_ptr");
    w->geometry.width = 1;
    w->geometry.height = 1;
    w->geometry_dirty = false;
    w->type = _NET_WM_WINDOW_TYPE_NORMAL;

    drawable_allocator(L, (drawable_refresh_callback *) drawin_refresh_pixmap, w);
    w->drawable = luaA_object_ref_item(L, -2, -1);

    w->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.default_depth, w->window, s->root,
                      w->geometry.x, w->geometry.y,
                      w->geometry.width, w->geometry.height,
                      w->border_width, XCB_COPY_FROM_PARENT, globalconf.visual->visual_id,
                      XCB_CW_BORDER_PIXEL | XCB_CW_BIT_GRAVITY
                      | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP
                      | XCB_CW_CURSOR,
                      (const uint32_t [])
                      {
                          w->border_color.pixel,
                          XCB_GRAVITY_NORTH_WEST,
                          1,
                          XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                          | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
                          | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                          | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS
                          | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_EXPOSURE
                          | XCB_EVENT_MASK_PROPERTY_CHANGE,
                          globalconf.default_cmap,
                          xcursor_new(globalconf.cursor_ctx, xcursor_font_fromstr(w->cursor))
                      });
    xwindow_set_class_instance(w->window);
    xwindow_set_name_static(w->window, "Awesome drawin");

    /* Set the right properties */
    ewmh_update_window_type(w->window, window_translate_type(w->type));
    ewmh_update_strut(w->window, &w->strut);

    return w;
}

/** Create a new drawin.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_new(lua_State *L)
{
    luaA_class_new(L, &drawin_class);

    return 1;
}

/** Get or set drawin geometry. That's the same as accessing or setting the x,
 * y, width or height properties of a drawin.
 *
 * @param A table with coordinates to modify.
 * @return A table with drawin coordinates and geometry.
 * @function geometry
 */
static int
luaA_drawin_geometry(lua_State *L)
{
    drawin_t *drawin = luaA_checkudata(L, 1, &drawin_class);

    if(lua_gettop(L) == 2)
    {
        area_t wingeom;

        luaA_checktable(L, 2);
        wingeom.x = round(luaA_getopt_number_range(L, 2, "x", drawin->geometry.x, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
        wingeom.y = round(luaA_getopt_number_range(L, 2, "y", drawin->geometry.y, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
        wingeom.width = ceil(luaA_getopt_number_range(L, 2, "width", drawin->geometry.width, MIN_X11_SIZE, MAX_X11_SIZE));
        wingeom.height = ceil(luaA_getopt_number_range(L, 2, "height", drawin->geometry.height, MIN_X11_SIZE, MAX_X11_SIZE));

        if(wingeom.width > 0 && wingeom.height > 0)
            drawin_moveresize(L, 1, wingeom);
    }

    return luaA_pusharea(L, drawin->geometry);
}


LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, ontop, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, cursor, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, visible, lua_pushboolean)

static int
luaA_drawin_set_x(lua_State *L, drawin_t *drawin)
{
    int x = round(luaA_checknumber_range(L, -1, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
    drawin_moveresize(L, -3, (area_t) { .x = x,
                                        .y = drawin->geometry.y,
                                        .width = drawin->geometry.width,
                                        .height = drawin->geometry.height });
    return 0;
}

static int
luaA_drawin_get_x(lua_State *L, drawin_t *drawin)
{
    lua_pushinteger(L, drawin->geometry.x);
    return 1;
}

static int
luaA_drawin_set_y(lua_State *L, drawin_t *drawin)
{
    int y = round(luaA_checknumber_range(L, -1, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
    drawin_moveresize(L, -3, (area_t) { .x = drawin->geometry.x,
                                        .y = y,
                                        .width = drawin->geometry.width,
                                        .height = drawin->geometry.height });
    return 0;
}

static int
luaA_drawin_get_y(lua_State *L, drawin_t *drawin)
{
    lua_pushinteger(L, drawin->geometry.y);
    return 1;
}

static int
luaA_drawin_set_width(lua_State *L, drawin_t *drawin)
{
    int width = ceil(luaA_checknumber_range(L, -1, MIN_X11_SIZE, MAX_X11_SIZE));
    drawin_moveresize(L, -3, (area_t) { .x = drawin->geometry.x,
                                        .y = drawin->geometry.y,
                                        .width = width,
                                        .height = drawin->geometry.height });
    return 0;
}

static int
luaA_drawin_get_width(lua_State *L, drawin_t *drawin)
{
    lua_pushinteger(L, drawin->geometry.width);
    return 1;
}

static int
luaA_drawin_set_height(lua_State *L, drawin_t *drawin)
{
    int height = ceil(luaA_checknumber_range(L, -1, MIN_X11_SIZE, MAX_X11_SIZE));
    drawin_moveresize(L, -3, (area_t) { .x = drawin->geometry.x,
                                       .y = drawin->geometry.y,
                                       .width = drawin->geometry.width,
                                       .height = height });
    return 0;
}

static int
luaA_drawin_get_height(lua_State *L, drawin_t *drawin)
{
    lua_pushinteger(L, drawin->geometry.height);
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
            xcb_cursor_t cursor = xcursor_new(globalconf.cursor_ctx, cursor_font);
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

/** Get a drawin's drawable
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_drawable(lua_State *L, drawin_t *drawin)
{
    luaA_object_push_item(L, -2, drawin->drawable);
    return 1;
}

/** Get the drawin's bounding shape.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_shape_bounding(lua_State *L, drawin_t *drawin)
{
    cairo_surface_t *surf = xwindow_get_shape(drawin->window, XCB_SHAPE_SK_BOUNDING);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Set the drawin's bounding shape.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_shape_bounding(lua_State *L, drawin_t *drawin)
{
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);

    /* The drawin might have been resized to a larger size. Apply that. */
    drawin_apply_moveresize(drawin);

    xwindow_set_shape(drawin->window,
            drawin->geometry.width + 2*drawin->border_width,
            drawin->geometry.height + 2*drawin->border_width,
            XCB_SHAPE_SK_BOUNDING, surf, -drawin->border_width);
    luaA_object_emit_signal(L, -3, "property::shape_bounding", 0);
    return 0;
}

/** Get the drawin's clip shape.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_shape_clip(lua_State *L, drawin_t *drawin)
{
    cairo_surface_t *surf = xwindow_get_shape(drawin->window, XCB_SHAPE_SK_CLIP);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Set the drawin's clip shape.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_shape_clip(lua_State *L, drawin_t *drawin)
{
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);

    /* The drawin might have been resized to a larger size. Apply that. */
    drawin_apply_moveresize(drawin);

    xwindow_set_shape(drawin->window, drawin->geometry.width, drawin->geometry.height,
            XCB_SHAPE_SK_CLIP, surf, 0);
    luaA_object_emit_signal(L, -3, "property::shape_clip", 0);
    return 0;
}

/** Get the drawin's input shape.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_shape_input(lua_State *L, drawin_t *drawin)
{
    cairo_surface_t *surf = xwindow_get_shape(drawin->window, XCB_SHAPE_SK_INPUT);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Set the drawin's input shape.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_shape_input(lua_State *L, drawin_t *drawin)
{
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);

    /* The drawin might have been resized to a larger size. Apply that. */
    drawin_apply_moveresize(drawin);

    xwindow_set_shape(drawin->window,
            drawin->geometry.width + 2*drawin->border_width,
            drawin->geometry.height + 2*drawin->border_width,
            XCB_SHAPE_SK_INPUT, surf, -drawin->border_width);
    luaA_object_emit_signal(L, -3, "property::shape_input", 0);
    return 0;
}

void
drawin_class_setup(lua_State *L)
{
    static const struct luaL_Reg drawin_methods[] =
    {
        LUA_CLASS_METHODS(drawin)
        { "get", luaA_drawin_get },
        { "__call", luaA_drawin_new },
        { NULL, NULL }
    };

    static const struct luaL_Reg drawin_meta[] =
    {
        LUA_OBJECT_META(drawin)
        LUA_CLASS_META
        { "geometry", luaA_drawin_geometry },
        { NULL, NULL },
    };

    luaA_class_setup(L, &drawin_class, "drawin", &window_class,
                     (lua_class_allocator_t) drawin_allocator,
                     (lua_class_collector_t) drawin_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     drawin_methods, drawin_meta);
    luaA_class_add_property(&drawin_class, "drawable",
                            NULL,
                            (lua_class_propfunc_t) luaA_drawin_get_drawable,
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
    luaA_class_add_property(&drawin_class, "type",
                            (lua_class_propfunc_t) luaA_window_set_type,
                            (lua_class_propfunc_t) luaA_window_get_type,
                            (lua_class_propfunc_t) luaA_window_set_type);
    luaA_class_add_property(&drawin_class, "shape_bounding",
                            (lua_class_propfunc_t) luaA_drawin_set_shape_bounding,
                            (lua_class_propfunc_t) luaA_drawin_get_shape_bounding,
                            (lua_class_propfunc_t) luaA_drawin_set_shape_bounding);
    luaA_class_add_property(&drawin_class, "shape_clip",
                            (lua_class_propfunc_t) luaA_drawin_set_shape_clip,
                            (lua_class_propfunc_t) luaA_drawin_get_shape_clip,
                            (lua_class_propfunc_t) luaA_drawin_set_shape_clip);
    luaA_class_add_property(&drawin_class, "shape_input",
                            (lua_class_propfunc_t) luaA_drawin_set_shape_input,
                            (lua_class_propfunc_t) luaA_drawin_get_shape_input,
                            (lua_class_propfunc_t) luaA_drawin_set_shape_input);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
