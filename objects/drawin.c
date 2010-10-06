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
#include "common/xcursor.h"
#include "common/xutil.h"

LUA_OBJECT_FUNCS(drawin_class, drawin_t, drawin)

/** Kick out systray windows.
 */
static void
drawin_systray_kickout(void)
{
    xcb_screen_t *s = globalconf.screen;

    if(globalconf.systray.parent != s->root)
    {
        /* Who! Check that we're not deleting a drawin with a systray, because it
         * may be its parent. If so, we reparent to root before, otherwise it will
         * hurt very much. */
        xcb_reparent_window(globalconf.connection,
                            globalconf.systray.window,
                            s->root, -512, -512);

        globalconf.systray.parent = s->root;
    }
}

/** Destroy all X resources of a drawin.
 * \param w The drawin to wipe.
 */
static void
drawin_wipe_resources(drawin_t *w)
{
    if(w->window)
    {
        /* Activate BMA */
        client_ignore_enterleave_events();
        /* Make sure we don't accidentally kill the systray window */
        if(globalconf.systray.parent == w->window)
            drawin_systray_kickout();
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
    draw_context_wipe(&w->ctx);
}

static void
drawin_wipe(drawin_t *drawin)
{
    p_delete(&drawin->cursor);
    drawin_wipe_resources(drawin);
    if(drawin->bg_image)
        cairo_surface_destroy(drawin->bg_image);
}

void
drawin_unref_simplified(drawin_t **item)
{
    luaA_object_unref(globalconf.L, *item);
}

static void
drawin_draw_context_update(drawin_t *w)
{
    xcb_screen_t *s = globalconf.screen;
    xcolor_t fg = w->ctx.fg, bg = w->ctx.bg;

    draw_context_wipe(&w->ctx);

    /* update draw context */
    switch(w->orientation)
    {
      case South:
      case North:
        /* we need a new pixmap this way [     ] to render */
        w->ctx.pixmap = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection,
                          globalconf.default_depth,
                          w->ctx.pixmap, s->root,
                          w->geometry.height,
                          w->geometry.width);
        draw_context_init(&w->ctx,
                          w->geometry.height,
                          w->geometry.width,
                          w->ctx.pixmap, &fg, &bg);
        break;
      case East:
        draw_context_init(&w->ctx,
                          w->geometry.width,
                          w->geometry.height,
                          w->pixmap, &fg, &bg);
        break;
    }
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
                      | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
                      (const uint32_t [])
                      {
                          w->ctx.bg.pixel,
                          w->border_color.pixel,
                          XCB_GRAVITY_NORTH_WEST,
                          1,
                          XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                          | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
                          | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                          | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE
                          | XCB_EVENT_MASK_PROPERTY_CHANGE,
                          globalconf.default_cmap
                      });

    /* Create a pixmap. */
    w->pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, globalconf.default_depth, w->pixmap, s->root,
                      w->geometry.width, w->geometry.height);

    /* Update draw context physical screen, important for Zaphod. */
    drawin_draw_context_update(w);

    /* Set the right type property */
    ewmh_update_window_type(w->window, window_translate_type(w->type));
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
    if(w->window)
    {
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
        {
            xcb_free_pixmap(globalconf.connection, w->pixmap);
            /* orientation != East */
            if(w->pixmap != w->ctx.pixmap)
                xcb_free_pixmap(globalconf.connection, w->ctx.pixmap);
            w->pixmap = xcb_generate_id(globalconf.connection);
            xcb_screen_t *s = globalconf.screen;
            xcb_create_pixmap(globalconf.connection, globalconf.default_depth, w->pixmap, s->root,
                              w->geometry.width, w->geometry.height);
            drawin_draw_context_update(w);
        }

        /* Activate BMA */
        client_ignore_enterleave_events();

        if(mask_vals)
            xcb_configure_window(globalconf.connection, w->window, mask_vals, moveresize_win_vals);

        /* Deactivate BMA */
        client_restore_enterleave_events();

        w->screen = screen_getbycoord(w->geometry.x, w->geometry.y);

        if(mask_vals & XCB_CONFIG_WINDOW_X)
            luaA_object_emit_signal(L, udx, "property::x", 0);
        if(mask_vals & XCB_CONFIG_WINDOW_Y)
            luaA_object_emit_signal(L, udx, "property::y", 0);
        if(mask_vals & XCB_CONFIG_WINDOW_WIDTH)
            luaA_object_emit_signal(L, udx, "property::width", 0);
        if(mask_vals & XCB_CONFIG_WINDOW_HEIGHT)
            luaA_object_emit_signal(L, udx, "property::height", 0);
    }
    else
    {
#define DO_drawin_GEOMETRY_CHECK_AND_EMIT(prop) \
        if(w->geometry.prop != geometry.prop) \
        { \
            w->geometry.prop = geometry.prop; \
            luaA_object_emit_signal(L, udx, "property::" #prop, 0); \
        }
        DO_drawin_GEOMETRY_CHECK_AND_EMIT(x)
        DO_drawin_GEOMETRY_CHECK_AND_EMIT(y)
        DO_drawin_GEOMETRY_CHECK_AND_EMIT(width)
        DO_drawin_GEOMETRY_CHECK_AND_EMIT(height)
#undef DO_drawin_GEOMETRY_CHECK_AND_EMIT
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
    xcb_copy_area(globalconf.connection, drawin->pixmap,
                  drawin->window, globalconf.gc, x, y, x, y,
                  w, h);
}

/** Set drawin orientation.
 * \param L The Lua VM state.
 * \param udx The drawin to change orientation.
 * \param o The new orientation.
 */
static void
drawin_set_orientation(lua_State *L, int udx, orientation_t o)
{
    drawin_t *w = luaA_checkudata(L, udx, &drawin_class);
    if(o != w->orientation)
    {
        w->orientation = o;
        /* orientation != East */
        if(w->pixmap != w->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, w->ctx.pixmap);
        drawin_draw_context_update(w);
        luaA_object_emit_signal(L, udx, "property::orientation", 0);
    }
}

static void
drawin_map(drawin_t *drawin)
{
    /* Activate BMA */
    client_ignore_enterleave_events();
    /* Map the drawin */
    xcb_map_window(globalconf.connection, drawin->window);
    /* Deactivate BMA */
    client_restore_enterleave_events();
    /* Stack this drawin correctly */
    stack_windows();
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

        if(drawin->screen)
        {
            if(drawin->visible)
                drawin_map(drawin);
            else
            {
                /* Active BMA */
                client_ignore_enterleave_events();
                /* Unmap window */
                xcb_unmap_window(globalconf.connection, drawin->window);
                /* Active BMA */
                client_restore_enterleave_events();
            }
        }

        luaA_object_emit_signal(L, udx, "property::visible", 0);
    }
}

/** Remove a drawin from a screen.
 * \param L The Lua VM state.
 * \param udx drawin to detach from screen.
 */
static void
drawin_detach(lua_State *L, int udx)
{
    drawin_t *drawin = luaA_checkudata(L, udx, &drawin_class);
    if(drawin->screen)
    {
        bool v;

        /* save visible state */
        v = drawin->visible;
        drawin->visible = false;
        /* restore visibility */
        drawin->visible = v;

        drawin_wipe_resources(drawin);

        foreach(item, globalconf.drawins)
            if(*item == drawin)
            {
                drawin_array_remove(&globalconf.drawins, item);
                break;
            }

        if(strut_has_value(&drawin->strut))
            screen_emit_signal(L, drawin->screen, "property::workarea", 0);

        drawin->screen = NULL;
        luaA_object_emit_signal(L, udx, "property::screen", 0);

        luaA_object_unref(globalconf.L, drawin);
    }
}

/** Attach a drawin that is on top of the stack.
 * \param L The Lua VM state.
 * \param udx The drawin to attach.
 * \param s The screen to attach the drawin to.
 */
static void
drawin_attach(lua_State *L, int udx, screen_t *s)
{
    /* duplicate drawin */
    lua_pushvalue(L, udx);
    /* ref it */
    drawin_t *drawin = luaA_object_ref_class(globalconf.L, -1, &drawin_class);

    drawin_detach(L, udx);

    /* Set the drawin screen */
    drawin->screen = s;

    /* Check that the drawin coordinates matches the screen. */
    screen_t *cscreen =
        screen_getbycoord(drawin->geometry.x, drawin->geometry.y);

    /* If it does not match, move it to the screen coordinates */
    if(cscreen != drawin->screen)
        drawin_moveresize(L, udx, (area_t) { .x = s->geometry.x,
                                            .y = s->geometry.y,
                                            .width = drawin->geometry.width,
                                            .height = drawin->geometry.height });

    drawin_array_append(&globalconf.drawins, drawin);

    drawin_init(drawin);

    xwindow_set_cursor(drawin->window,
                      xcursor_new(globalconf.connection, xcursor_font_fromstr(drawin->cursor)));

    if(drawin->opacity != -1)
        xwindow_set_opacity(drawin->window, drawin->opacity);

    ewmh_update_strut(drawin->window, &drawin->strut);

    if(drawin->visible)
        drawin_map(drawin);

    luaA_object_emit_signal(L, udx, "property::screen", 0);

    if(strut_has_value(&drawin->strut))
        screen_emit_signal(L, drawin->screen, "property::workarea", 0);
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

    if(!w->ctx.fg.initialized)
        w->ctx.fg = globalconf.colors.fg;

    if(!w->ctx.bg.initialized)
        w->ctx.bg = globalconf.colors.bg;

    w->visible = true;

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

LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, ontop, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, cursor, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(drawin, drawin_t, visible, lua_pushboolean)

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

/** Set the drawin foreground color.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_fg(lua_State *L, drawin_t *drawin)
{
    size_t len;
    const char *buf = luaL_checklstring(L, -1, &len);
    xcolor_init_reply(xcolor_init_unchecked(&drawin->ctx.fg, buf, len));
    luaA_object_emit_signal(L, -3, "property::fg", 0);
    return 0;
}

/** Get the drawin foreground color.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_fg(lua_State *L, drawin_t *drawin)
{
    return luaA_pushxcolor(L, drawin->ctx.fg);
}

/** Set the drawin background color.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_bg(lua_State *L, drawin_t *drawin)
{
    size_t len;
    const char *buf = luaL_checklstring(L, -1, &len);
    if(xcolor_init_reply(xcolor_init_unchecked(&drawin->ctx.bg, buf, len)))
    {
        uint32_t mask = XCB_CW_BACK_PIXEL;
        uint32_t values[] = { drawin->ctx.bg.pixel };

        if (drawin->window != XCB_NONE)
            xcb_change_window_attributes(globalconf.connection,
                                         drawin->window,
                                         mask,
                                         values);
    }
    luaA_object_emit_signal(L, -3, "property::bg", 0);
    return 0;
}

/** Get the drawin background color.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_bg(lua_State *L, drawin_t *drawin)
{
    return luaA_pushxcolor(L, drawin->ctx.bg);
}

/** Set the drawin background image.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_bg_image(lua_State *L, drawin_t *drawin)
{
    if(lua_isnil(L, -1))
    {
        if(drawin->bg_image)
            cairo_surface_destroy(drawin->bg_image);
        drawin->bg_image = NULL;
    } else {
        cairo_surface_t **cairo_surface = (cairo_surface_t **)luaL_checkudata(L, -1, OOCAIRO_MT_NAME_SURFACE);
        if(drawin->bg_image)
            cairo_surface_destroy(drawin->bg_image);
        drawin->bg_image = draw_dup_image_surface(*cairo_surface);
    }
    luaA_object_emit_signal(L, -3, "property::bg_image", 0);
    return 0;
}

/** Get the drawin background image.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_bg_image(lua_State *L, drawin_t *drawin)
{
    if(drawin->bg_image)
        return oocairo_surface_push(L, drawin->bg_image);
    return 0;
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

/** Set the drawin screen.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_screen(lua_State *L, drawin_t *drawin)
{
    if(lua_isnil(L, -1))
        drawin_detach(L, -3);
    else
    {
        int screen = luaL_checknumber(L, -1) - 1;
        luaA_checkscreen(screen);
        if(!drawin->screen || screen != screen_array_indexof(&globalconf.screens, drawin->screen))
            drawin_attach(L, -3, &globalconf.screens.tab[screen]);
    }
    return 0;
}

/** Get the drawin screen.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_screen(lua_State *L, drawin_t *drawin)
{
    if(!drawin->screen)
        return 0;
    lua_pushnumber(L, screen_array_indexof(&globalconf.screens, drawin->screen) + 1);
    return 1;
}

/** Set the drawin orientation.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_set_orientation(lua_State *L, drawin_t *drawin)
{
    const char *buf = luaL_checkstring(L, -1);
    if(buf)
    {
        drawin_set_orientation(L, -3, orientation_fromstr(buf));
    }
    return 0;
}

/** Get the drawin orientation.
 * \param L The Lua VM state.
 * \param drawin The drawin object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_drawin_get_orientation(lua_State *L, drawin_t *drawin)
{
    lua_pushstring(L, orientation_tostr(drawin->orientation));
    return 1;
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
        { NULL, NULL },
    };

    luaA_class_setup(L, &drawin_class, "drawin", &window_class,
                     (lua_class_allocator_t) drawin_new,
                     (lua_class_collector_t) drawin_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     drawin_methods, drawin_meta);
    luaA_class_add_property(&drawin_class, "visible",
                            (lua_class_propfunc_t) luaA_drawin_set_visible,
                            (lua_class_propfunc_t) luaA_drawin_get_visible,
                            (lua_class_propfunc_t) luaA_drawin_set_visible);
    luaA_class_add_property(&drawin_class, "orientation",
                            (lua_class_propfunc_t) luaA_drawin_set_orientation,
                            (lua_class_propfunc_t) luaA_drawin_get_orientation,
                            (lua_class_propfunc_t) luaA_drawin_set_orientation);
    luaA_class_add_property(&drawin_class, "ontop",
                            (lua_class_propfunc_t) luaA_drawin_set_ontop,
                            (lua_class_propfunc_t) luaA_drawin_get_ontop,
                            (lua_class_propfunc_t) luaA_drawin_set_ontop);
    luaA_class_add_property(&drawin_class, "screen",
                            NULL,
                            (lua_class_propfunc_t) luaA_drawin_get_screen,
                            (lua_class_propfunc_t) luaA_drawin_set_screen);
    luaA_class_add_property(&drawin_class, "cursor",
                            (lua_class_propfunc_t) luaA_drawin_set_cursor,
                            (lua_class_propfunc_t) luaA_drawin_get_cursor,
                            (lua_class_propfunc_t) luaA_drawin_set_cursor);
    luaA_class_add_property(&drawin_class, "fg",
                            (lua_class_propfunc_t) luaA_drawin_set_fg,
                            (lua_class_propfunc_t) luaA_drawin_get_fg,
                            (lua_class_propfunc_t) luaA_drawin_set_fg);
    luaA_class_add_property(&drawin_class, "bg",
                            (lua_class_propfunc_t) luaA_drawin_set_bg,
                            (lua_class_propfunc_t) luaA_drawin_get_bg,
                            (lua_class_propfunc_t) luaA_drawin_set_bg);
    luaA_class_add_property(&drawin_class, "bg_image",
                            (lua_class_propfunc_t) luaA_drawin_set_bg_image,
                            (lua_class_propfunc_t) luaA_drawin_get_bg_image,
                            (lua_class_propfunc_t) luaA_drawin_set_bg_image);
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

    signal_add(&drawin_class.signals, "mouse::enter");
    signal_add(&drawin_class.signals, "mouse::leave");
    signal_add(&drawin_class.signals, "property::bg");
    signal_add(&drawin_class.signals, "property::bg_image");
    signal_add(&drawin_class.signals, "property::border_width");
    signal_add(&drawin_class.signals, "property::cursor");
    signal_add(&drawin_class.signals, "property::fg");
    signal_add(&drawin_class.signals, "property::height");
    signal_add(&drawin_class.signals, "property::ontop");
    signal_add(&drawin_class.signals, "property::orientation");
    signal_add(&drawin_class.signals, "property::screen");
    signal_add(&drawin_class.signals, "property::visible");
    signal_add(&drawin_class.signals, "property::widgets");
    signal_add(&drawin_class.signals, "property::width");
    signal_add(&drawin_class.signals, "property::x");
    signal_add(&drawin_class.signals, "property::y");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
