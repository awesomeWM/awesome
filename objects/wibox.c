/*
 * wibox.c - wibox functions
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

#include "screen.h"
#include "wibox.h"
#include "objects/client.h"
#include "screen.h"
#include "xwindow.h"
#include "luaa.h"
#include "ewmh.h"
#include "common/xcursor.h"
#include "common/xutil.h"

LUA_OBJECT_FUNCS(wibox_class, wibox_t, wibox)

/** Kick out systray windows.
 */
static void
wibox_systray_kickout(void)
{
    xcb_screen_t *s = globalconf.screen;

    if(globalconf.systray.parent != s->root)
    {
        /* Who! Check that we're not deleting a wibox with a systray, because it
         * may be its parent. If so, we reparent to root before, otherwise it will
         * hurt very much. */
        xcb_reparent_window(globalconf.connection,
                            globalconf.systray.window,
                            s->root, -512, -512);

        globalconf.systray.parent = s->root;
    }
}

/** Destroy all X resources of a wibox.
 * \param w The wibox to wipe.
 */
static void
wibox_wipe_resources(wibox_t *w)
{
    if(w->window)
    {
        /* Activate BMA */
        client_ignore_enterleave_events();
        /* Make sure we don't accidentally kill the systray window */
        if(globalconf.systray.parent == w->window)
            wibox_systray_kickout();
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
wibox_wipe(wibox_t *wibox)
{
    p_delete(&wibox->cursor);
    wibox_wipe_resources(wibox);
    widget_node_array_wipe(&wibox->widgets);
    if(wibox->bg_image)
        cairo_surface_destroy(wibox->bg_image);
}

/** Wipe an array of widget_node. Release references to widgets.
 * \param L The Lua VM state.
 * \param idx The index of the wibox on the stack.
 */
void
wibox_widget_node_array_wipe(lua_State *L, int idx)
{
    wibox_t *wibox = luaA_checkudata(L, idx, &wibox_class);
    foreach(widget_node, wibox->widgets)
        luaA_object_unref_item(globalconf.L, idx, widget_node->widget);
    widget_node_array_wipe(&wibox->widgets);
}


void
wibox_unref_simplified(wibox_t **item)
{
    luaA_object_unref(globalconf.L, *item);
}

static void
wibox_need_update(wibox_t *wibox)
{
    wibox->need_update = true;
    wibox_clear_mouse_over(wibox);
}

static void
wibox_draw_context_update(wibox_t *w)
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
                          s->root_depth,
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

/** Initialize a wibox.
 * \param w The wibox to initialize.
 */
static void
wibox_init(wibox_t *w)
{
    xcb_screen_t *s = globalconf.screen;

    w->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, s->root_depth, w->window, s->root,
                      w->geometry.x, w->geometry.y,
                      w->geometry.width, w->geometry.height,
                      w->border_width, XCB_COPY_FROM_PARENT, s->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_BIT_GRAVITY
                      | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
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
                          | XCB_EVENT_MASK_PROPERTY_CHANGE
                      });

    /* Create a pixmap. */
    w->pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, s->root_depth, w->pixmap, s->root,
                      w->geometry.width, w->geometry.height);

    /* Update draw context physical screen, important for Zaphod. */
    wibox_draw_context_update(w);

    /* Set the right type property */
    ewmh_update_window_type(w->window, window_translate_type(w->type));
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param w The wibox to refresh.
 */
static inline void
wibox_refresh_pixmap(wibox_t *w)
{
    wibox_refresh_pixmap_partial(w, 0, 0, w->geometry.width, w->geometry.height);
}

/** Move and/or resize a wibox
 * \param L The Lua VM state.
 * \param udx The index of the wibox.
 * \param geometry The new geometry.
 */
static void
wibox_moveresize(lua_State *L, int udx, area_t geometry)
{
    wibox_t *w = luaA_checkudata(L, udx, &wibox_class);
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
            xcb_create_pixmap(globalconf.connection, s->root_depth, w->pixmap, s->root,
                              w->geometry.width, w->geometry.height);
            wibox_draw_context_update(w);
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
#define DO_WIBOX_GEOMETRY_CHECK_AND_EMIT(prop) \
        if(w->geometry.prop != geometry.prop) \
        { \
            w->geometry.prop = geometry.prop; \
            luaA_object_emit_signal(L, udx, "property::" #prop, 0); \
        }
        DO_WIBOX_GEOMETRY_CHECK_AND_EMIT(x)
        DO_WIBOX_GEOMETRY_CHECK_AND_EMIT(y)
        DO_WIBOX_GEOMETRY_CHECK_AND_EMIT(width)
        DO_WIBOX_GEOMETRY_CHECK_AND_EMIT(height)
#undef DO_WIBOX_GEOMETRY_CHECK_AND_EMIT
    }

    wibox_need_update(w);
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param wibox The wibox to refresh.
 * \param x The copy starting point x component.
 * \param y The copy starting point y component.
 * \param w The copy width from the x component.
 * \param h The copy height from the y component.
 */
void
wibox_refresh_pixmap_partial(wibox_t *wibox,
                             int16_t x, int16_t y,
                             uint16_t w, uint16_t h)
{
    xcb_copy_area(globalconf.connection, wibox->pixmap,
                  wibox->window, globalconf.gc, x, y, x, y,
                  w, h);
}

/** Set wibox orientation.
 * \param L The Lua VM state.
 * \param udx The wibox to change orientation.
 * \param o The new orientation.
 */
static void
wibox_set_orientation(lua_State *L, int udx, orientation_t o)
{
    wibox_t *w = luaA_checkudata(L, udx, &wibox_class);
    if(o != w->orientation)
    {
        w->orientation = o;
        /* orientation != East */
        if(w->pixmap != w->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, w->ctx.pixmap);
        wibox_draw_context_update(w);
        luaA_object_emit_signal(L, udx, "property::orientation", 0);
    }
}

static void
wibox_map(wibox_t *wibox)
{
    /* Activate BMA */
    client_ignore_enterleave_events();
    /* Map the wibox */
    xcb_map_window(globalconf.connection, wibox->window);
    /* Deactivate BMA */
    client_restore_enterleave_events();
    /* We must make sure the wibox does not display garbage */
    wibox_need_update(wibox);
    /* Stack this wibox correctly */
    stack_windows();
}

static void
wibox_systray_refresh(wibox_t *wibox)
{
    wibox->has_systray = false;

    if(!wibox->screen)
        return;

    for(int i = 0; i < wibox->widgets.len; i++)
    {
        widget_node_t *systray = &wibox->widgets.tab[i];
        if(systray->widget->type == widget_systray)
        {
            uint32_t config_back[] = { wibox->ctx.bg.pixel };
            uint32_t config_win_vals[4];
            uint32_t config_win_vals_off[2] = { -512, -512 };
            xembed_window_t *em;

            wibox->has_systray = true;

            if(wibox->visible
               && systray->widget->isvisible
               && systray->geometry.width)
            {
                /* Set background of the systray window. */
                xcb_change_window_attributes(globalconf.connection,
                                             globalconf.systray.window,
                                             XCB_CW_BACK_PIXEL, config_back);
                /* Map it. */
                xcb_map_window(globalconf.connection, globalconf.systray.window);
                /* Move it. */
                switch(wibox->orientation)
                {
                  case North:
                    config_win_vals[0] = systray->geometry.y;
                    config_win_vals[1] = wibox->geometry.height - systray->geometry.x - systray->geometry.width;
                    config_win_vals[2] = systray->geometry.height;
                    config_win_vals[3] = systray->geometry.width;
                    break;
                  case South:
                    config_win_vals[0] = systray->geometry.y;
                    config_win_vals[1] = systray->geometry.x;
                    config_win_vals[2] = systray->geometry.height;
                    config_win_vals[3] = systray->geometry.width;
                    break;
                  case East:
                    config_win_vals[0] = systray->geometry.x;
                    config_win_vals[1] = systray->geometry.y;
                    config_win_vals[2] = systray->geometry.width;
                    config_win_vals[3] = systray->geometry.height;
                    break;
                }
                /* reparent */
                if(globalconf.systray.parent != wibox->window)
                {
                    xcb_reparent_window(globalconf.connection,
                                        globalconf.systray.window,
                                        wibox->window,
                                        config_win_vals[0], config_win_vals[1]);
                    globalconf.systray.parent = wibox->window;
                }
                xcb_configure_window(globalconf.connection,
                                     globalconf.systray.window,
                                     XCB_CONFIG_WINDOW_X
                                     | XCB_CONFIG_WINDOW_Y
                                     | XCB_CONFIG_WINDOW_WIDTH
                                     | XCB_CONFIG_WINDOW_HEIGHT,
                                     config_win_vals);
                /* width = height = systray height */
                config_win_vals[2] = config_win_vals[3] = systray->geometry.height;
                config_win_vals[0] = 0;
            }
            else
                return wibox_systray_kickout();

            switch(wibox->orientation)
            {
              case North:
                config_win_vals[1] = systray->geometry.width - config_win_vals[3];
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
                    if(config_win_vals[1] - config_win_vals[2] >= (uint32_t) wibox->geometry.y)
                    {
                        xcb_map_window(globalconf.connection, em->win);
                        xcb_configure_window(globalconf.connection, em->win,
                                             XCB_CONFIG_WINDOW_X
                                             | XCB_CONFIG_WINDOW_Y
                                             | XCB_CONFIG_WINDOW_WIDTH
                                             | XCB_CONFIG_WINDOW_HEIGHT,
                                             config_win_vals);
                        config_win_vals[1] -= config_win_vals[3];
                    }
                    else
                        xcb_configure_window(globalconf.connection, em->win,
                                             XCB_CONFIG_WINDOW_X
                                             | XCB_CONFIG_WINDOW_Y,
                                             config_win_vals_off);
                }
                break;
              case South:
                config_win_vals[1] = 0;
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
                    /* if(y + width <= wibox.y + systray.right) */
                    if(config_win_vals[1] + config_win_vals[3] <= (uint32_t) wibox->geometry.y + AREA_RIGHT(systray->geometry))
                    {
                        xcb_map_window(globalconf.connection, em->win);
                        xcb_configure_window(globalconf.connection, em->win,
                                             XCB_CONFIG_WINDOW_X
                                             | XCB_CONFIG_WINDOW_Y
                                             | XCB_CONFIG_WINDOW_WIDTH
                                             | XCB_CONFIG_WINDOW_HEIGHT,
                                             config_win_vals);
                        config_win_vals[1] += config_win_vals[3];
                    }
                    else
                        xcb_configure_window(globalconf.connection, em->win,
                                             XCB_CONFIG_WINDOW_X
                                             | XCB_CONFIG_WINDOW_Y,
                                             config_win_vals_off);
                }
                break;
              case East:
                config_win_vals[1] = 0;
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
                    /* if(x + width < systray.x + systray.width) */
                    if(config_win_vals[0] + config_win_vals[2] <= (uint32_t) AREA_RIGHT(systray->geometry) + wibox->geometry.x)
                    {
                        xcb_map_window(globalconf.connection, em->win);
                        xcb_configure_window(globalconf.connection, em->win,
                                             XCB_CONFIG_WINDOW_X
                                             | XCB_CONFIG_WINDOW_Y
                                             | XCB_CONFIG_WINDOW_WIDTH
                                             | XCB_CONFIG_WINDOW_HEIGHT,
                                             config_win_vals);
                        config_win_vals[0] += config_win_vals[2];
                    }
                    else
                        xcb_configure_window(globalconf.connection, em->win,
                                             XCB_CONFIG_WINDOW_X
                                             | XCB_CONFIG_WINDOW_Y,
                                             config_win_vals_off);
                }
                break;
            }
            break;
        }
    }
}

/** Get a wibox by its window.
 * \param win The window id.
 * \return A wibox if found, NULL otherwise.
 */
wibox_t *
wibox_getbywin(xcb_window_t win)
{
    foreach(w, globalconf.wiboxes)
        if((*w)->window == win)
            return *w;
    return NULL;
}

/** Draw a wibox.
 * \param wibox The wibox to draw.
 */
static void
wibox_draw(wibox_t *wibox)
{
    if(wibox->visible)
    {
        widget_render(wibox);
        wibox_refresh_pixmap(wibox);

        wibox->need_update = false;
    }

    wibox_systray_refresh(wibox);
}

/** Refresh all wiboxes.
 */
void
wibox_refresh(void)
{
    foreach(w, globalconf.wiboxes)
    {
        if((*w)->need_update)
            wibox_draw(*w);
    }
}

/** Clear the wibox' mouse_over pointer.
 * \param wibox The wibox.
 */
void
wibox_clear_mouse_over(wibox_t *wibox)
{
    if (wibox->mouse_over)
    {
        luaA_object_unref(globalconf.L, wibox->mouse_over);
        wibox->mouse_over = NULL;
    }
}

/** Set a wibox visible or not.
 * \param L The Lua VM state.
 * \param udx The wibox.
 * \param v The visible value.
 */
static void
wibox_set_visible(lua_State *L, int udx, bool v)
{
    wibox_t *wibox = luaA_checkudata(L, udx, &wibox_class);
    if(v != wibox->visible)
    {
        wibox->visible = v;
        wibox_clear_mouse_over(wibox);

        if(wibox->screen)
        {
            if(wibox->visible)
                wibox_map(wibox);
            else
            {
                /* Active BMA */
                client_ignore_enterleave_events();
                /* Unmap window */
                xcb_unmap_window(globalconf.connection, wibox->window);
                /* Active BMA */
                client_restore_enterleave_events();
            }

            /* kick out systray if needed */
            wibox_systray_refresh(wibox);
        }

        luaA_object_emit_signal(L, udx, "property::visible", 0);
    }
}

/** Remove a wibox from a screen.
 * \param L The Lua VM state.
 * \param udx Wibox to detach from screen.
 */
static void
wibox_detach(lua_State *L, int udx)
{
    wibox_t *wibox = luaA_checkudata(L, udx, &wibox_class);
    if(wibox->screen)
    {
        bool v;

        /* save visible state */
        v = wibox->visible;
        wibox->visible = false;
        wibox_systray_refresh(wibox);
        /* restore visibility */
        wibox->visible = v;

        wibox_clear_mouse_over(wibox);

        wibox_wipe_resources(wibox);

        foreach(item, globalconf.wiboxes)
            if(*item == wibox)
            {
                wibox_array_remove(&globalconf.wiboxes, item);
                break;
            }

        if(strut_has_value(&wibox->strut))
            screen_emit_signal(L, wibox->screen, "property::workarea", 0);

        wibox->screen = NULL;
        luaA_object_emit_signal(L, udx, "property::screen", 0);

        luaA_object_unref(globalconf.L, wibox);
    }
}

/** Attach a wibox that is on top of the stack.
 * \param L The Lua VM state.
 * \param udx The wibox to attach.
 * \param s The screen to attach the wibox to.
 */
static void
wibox_attach(lua_State *L, int udx, screen_t *s)
{
    /* duplicate wibox */
    lua_pushvalue(L, udx);
    /* ref it */
    wibox_t *wibox = luaA_object_ref_class(globalconf.L, -1, &wibox_class);

    wibox_detach(L, udx);

    /* Set the wibox screen */
    wibox->screen = s;

    /* Check that the wibox coordinates matches the screen. */
    screen_t *cscreen =
        screen_getbycoord(wibox->geometry.x, wibox->geometry.y);

    /* If it does not match, move it to the screen coordinates */
    if(cscreen != wibox->screen)
        wibox_moveresize(L, udx, (area_t) { .x = s->geometry.x,
                                            .y = s->geometry.y,
                                            .width = wibox->geometry.width,
                                            .height = wibox->geometry.height });

    wibox_array_append(&globalconf.wiboxes, wibox);

    wibox_init(wibox);

    xwindow_set_cursor(wibox->window,
                      xcursor_new(globalconf.connection, xcursor_font_fromstr(wibox->cursor)));

    if(wibox->opacity != -1)
        xwindow_set_opacity(wibox->window, wibox->opacity);

    ewmh_update_strut(wibox->window, &wibox->strut);

    if(wibox->visible)
        wibox_map(wibox);
    else
        wibox_need_update(wibox);

    luaA_object_emit_signal(L, udx, "property::screen", 0);

    if(strut_has_value(&wibox->strut))
        screen_emit_signal(L, wibox->screen, "property::workarea", 0);
}

static int
luaA_wibox_need_update(lua_State *L)
{
    wibox_need_update(luaA_checkudata(L, 1, &wibox_class));
    return 0;
}

/** Create a new wibox.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_new(lua_State *L)
{
    luaA_class_new(L, &wibox_class);

    wibox_t *w = luaA_checkudata(L, -1, &wibox_class);

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

    luaA_object_connect_signal(L, -2, "property::border_width", luaA_wibox_need_update);

    return 1;
}

/** Check if a wibox widget table has an item.
 * \param L The Lua VM state.
 * \param wibox The wibox.
 * \param item The item to look for.
 */
static bool
luaA_wibox_hasitem(lua_State *L, wibox_t *wibox, const void *item)
{
    if(wibox->widgets_table)
    {
        luaA_object_push(L, wibox);
        luaA_object_push_item(L, -1, wibox->widgets_table);
        lua_remove(L, -2);
        if(lua_topointer(L, -1) == item || luaA_hasitem(L, item))
            return true;
    }
    return false;
}

/** Invalidate a wibox by a Lua object (table, etc).
 * \param L The Lua VM state.
 * \param item The object identifier.
 */
void
luaA_wibox_invalidate_byitem(lua_State *L, const void *item)
{
    foreach(w, globalconf.wiboxes)
    {
        wibox_t *wibox = *w;
        if(luaA_wibox_hasitem(L, wibox, item))
        {
            /* update wibox */
            wibox_need_update(wibox);
            lua_pop(L, 1); /* remove widgets table */
        }

    }
}

/* Set or get the wibox geometry.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam An optional table with wibox geometry.
 * \lreturn The wibox geometry.
 */
static int
luaA_wibox_geometry(lua_State *L)
{
    wibox_t *wibox = luaA_checkudata(L, 1, &wibox_class);

    if(lua_gettop(L) == 2)
    {
        area_t wingeom;

        luaA_checktable(L, 2);
        wingeom.x = luaA_getopt_number(L, 2, "x", wibox->geometry.x);
        wingeom.y = luaA_getopt_number(L, 2, "y", wibox->geometry.y);
        wingeom.width = luaA_getopt_number(L, 2, "width", wibox->geometry.width);
        wingeom.height = luaA_getopt_number(L, 2, "height", wibox->geometry.height);

        if(wingeom.width > 0 && wingeom.height > 0)
            wibox_moveresize(L, 1, wingeom);
    }

    return luaA_pusharea(L, wibox->geometry);
}

LUA_OBJECT_EXPORT_PROPERTY(wibox, wibox_t, ontop, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(wibox, wibox_t, cursor, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(wibox, wibox_t, visible, lua_pushboolean)

static int
luaA_wibox_set_x(lua_State *L, wibox_t *wibox)
{
    wibox_moveresize(L, -3, (area_t) { .x = luaL_checknumber(L, -1),
                                       .y = wibox->geometry.y,
                                       .width = wibox->geometry.width,
                                       .height = wibox->geometry.height });
    return 0;
}

static int
luaA_wibox_get_x(lua_State *L, wibox_t *wibox)
{
    lua_pushnumber(L, wibox->geometry.x);
    return 1;
}

static int
luaA_wibox_set_y(lua_State *L, wibox_t *wibox)
{
    wibox_moveresize(L, -3, (area_t) { .x = wibox->geometry.x,
                                       .y = luaL_checknumber(L, -1),
                                       .width = wibox->geometry.width,
                                       .height = wibox->geometry.height });
    return 0;
}

static int
luaA_wibox_get_y(lua_State *L, wibox_t *wibox)
{
    lua_pushnumber(L, wibox->geometry.y);
    return 1;
}

static int
luaA_wibox_set_width(lua_State *L, wibox_t *wibox)
{
    int width = luaL_checknumber(L, -1);
    if(width <= 0)
        luaL_error(L, "invalid width");
    wibox_moveresize(L, -3, (area_t) { .x = wibox->geometry.x,
                                       .y = wibox->geometry.y,
                                       .width = width,
                                       .height = wibox->geometry.height });
    return 0;
}

static int
luaA_wibox_get_width(lua_State *L, wibox_t *wibox)
{
    lua_pushnumber(L, wibox->geometry.width);
    return 1;
}

static int
luaA_wibox_set_height(lua_State *L, wibox_t *wibox)
{
    int height = luaL_checknumber(L, -1);
    if(height <= 0)
        luaL_error(L, "invalid height");
    wibox_moveresize(L, -3, (area_t) { .x = wibox->geometry.x,
                                       .y = wibox->geometry.y,
                                       .width = wibox->geometry.width,
                                       .height = height });
    return 0;
}

static int
luaA_wibox_get_height(lua_State *L, wibox_t *wibox)
{
    lua_pushnumber(L, wibox->geometry.height);
    return 1;
}

/** Set the wibox foreground color.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_fg(lua_State *L, wibox_t *wibox)
{
    size_t len;
    const char *buf = luaL_checklstring(L, -1, &len);
    if(xcolor_init_reply(xcolor_init_unchecked(&wibox->ctx.fg, buf, len)))
        wibox->need_update = true;
    luaA_object_emit_signal(L, -3, "property::fg", 0);
    return 0;
}

/** Get the wibox foreground color.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_get_fg(lua_State *L, wibox_t *wibox)
{
    return luaA_pushxcolor(L, wibox->ctx.fg);
}

/** Set the wibox background color.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_bg(lua_State *L, wibox_t *wibox)
{
    size_t len;
    const char *buf = luaL_checklstring(L, -1, &len);
    if(xcolor_init_reply(xcolor_init_unchecked(&wibox->ctx.bg, buf, len)))
    {
        uint32_t mask = XCB_CW_BACK_PIXEL;
        uint32_t values[] = { wibox->ctx.bg.pixel };

        wibox->need_update = true;

        if (wibox->window != XCB_NONE)
            xcb_change_window_attributes(globalconf.connection,
                                         wibox->window,
                                         mask,
                                         values);
    }
    luaA_object_emit_signal(L, -3, "property::bg", 0);
    return 0;
}

/** Get the wibox background color.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_get_bg(lua_State *L, wibox_t *wibox)
{
    return luaA_pushxcolor(L, wibox->ctx.bg);
}

/** Set the wibox background image.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_bg_image(lua_State *L, wibox_t *wibox)
{
    if(lua_isnil(L, -1))
    {
        if(wibox->bg_image)
            cairo_surface_destroy(wibox->bg_image);
        wibox->bg_image = NULL;
    } else {
        cairo_surface_t **cairo_surface = (cairo_surface_t **)luaL_checkudata(L, -1, OOCAIRO_MT_NAME_SURFACE);
        if(wibox->bg_image)
            cairo_surface_destroy(wibox->bg_image);
        wibox->bg_image = draw_dup_image_surface(*cairo_surface);
    }
    wibox->need_update = true;
    luaA_object_emit_signal(L, -3, "property::bg_image", 0);
    return 0;
}

/** Get the wibox background image.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_get_bg_image(lua_State *L, wibox_t *wibox)
{
    if(wibox->bg_image)
        return oocairo_surface_push(L, wibox->bg_image);
    return 0;
}

/** Set the wibox on top status.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_ontop(lua_State *L, wibox_t *wibox)
{
    bool b = luaA_checkboolean(L, -1);
    if(b != wibox->ontop)
    {
        wibox->ontop = b;
        stack_windows();
        luaA_object_emit_signal(L, -3, "property::ontop", 0);
    }
    return 0;
}

/** Set the wibox cursor.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_cursor(lua_State *L, wibox_t *wibox)
{
    const char *buf = luaL_checkstring(L, -1);
    if(buf)
    {
        uint16_t cursor_font = xcursor_font_fromstr(buf);
        if(cursor_font)
        {
            xcb_cursor_t cursor = xcursor_new(globalconf.connection, cursor_font);
            p_delete(&wibox->cursor);
            wibox->cursor = a_strdup(buf);
            xwindow_set_cursor(wibox->window, cursor);
            luaA_object_emit_signal(L, -3, "property::cursor", 0);
        }
    }
    return 0;
}

/** Set the wibox screen.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_screen(lua_State *L, wibox_t *wibox)
{
    if(lua_isnil(L, -1))
        wibox_detach(L, -3);
    else
    {
        int screen = luaL_checknumber(L, -1) - 1;
        luaA_checkscreen(screen);
        if(!wibox->screen || screen != screen_array_indexof(&globalconf.screens, wibox->screen))
            wibox_attach(L, -3, &globalconf.screens.tab[screen]);
    }
    return 0;
}

/** Get the wibox screen.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_get_screen(lua_State *L, wibox_t *wibox)
{
    if(!wibox->screen)
        return 0;
    lua_pushnumber(L, screen_array_indexof(&globalconf.screens, wibox->screen) + 1);
    return 1;
}

/** Set the wibox orientation.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_orientation(lua_State *L, wibox_t *wibox)
{
    const char *buf = luaL_checkstring(L, -1);
    if(buf)
    {
        wibox_set_orientation(L, -3, orientation_fromstr(buf));
        wibox_need_update(wibox);
    }
    return 0;
}

/** Get the wibox orientation.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_get_orientation(lua_State *L, wibox_t *wibox)
{
    lua_pushstring(L, orientation_tostr(wibox->orientation));
    return 1;
}

/** Set the wibox visibility.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_visible(lua_State *L, wibox_t *wibox)
{
    wibox_set_visible(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

/** Set the wibox widgets.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_set_widgets(lua_State *L, wibox_t *wibox)
{
    if(luaA_isloop(L, -1))
    {
        luaA_warn(L, "table is looping, cannot use this as widget table");
        return 0;
    }
    /* duplicate table because next function will eat it */
    lua_pushvalue(L, -1);
    luaA_object_unref_item(L, -4, wibox->widgets_table);
    wibox->widgets_table = luaA_object_ref_item(L, -4, -1);
    luaA_object_emit_signal(L, -3, "property::widgets", 0);
    wibox_need_update(wibox);
    luaA_table2wtable(L);
    return 0;
}

/** Get the wibox widgets.
 * \param L The Lua VM state.
 * \param wibox The wibox object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_get_widgets(lua_State *L, wibox_t *wibox)
{
    return luaA_object_push_item(L, 1, wibox->widgets_table);
}
void
wibox_class_setup(lua_State *L)
{
    static const struct luaL_reg wibox_methods[] =
    {
        LUA_CLASS_METHODS(wibox)
        { "__call", luaA_wibox_new },
        { NULL, NULL }
    };

    static const struct luaL_reg wibox_meta[] =
    {
        LUA_OBJECT_META(wibox)
        LUA_CLASS_META
        { "geometry", luaA_wibox_geometry },
        { NULL, NULL },
    };

    luaA_class_setup(L, &wibox_class, "wibox", &window_class,
                     (lua_class_allocator_t) wibox_new,
                     (lua_class_collector_t) wibox_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     wibox_methods, wibox_meta);
    luaA_class_add_property(&wibox_class, "widgets",
                            (lua_class_propfunc_t) luaA_wibox_set_widgets,
                            (lua_class_propfunc_t) luaA_wibox_get_widgets,
                            (lua_class_propfunc_t) luaA_wibox_set_widgets);
    luaA_class_add_property(&wibox_class, "visible",
                            (lua_class_propfunc_t) luaA_wibox_set_visible,
                            (lua_class_propfunc_t) luaA_wibox_get_visible,
                            (lua_class_propfunc_t) luaA_wibox_set_visible);
    luaA_class_add_property(&wibox_class, "orientation",
                            (lua_class_propfunc_t) luaA_wibox_set_orientation,
                            (lua_class_propfunc_t) luaA_wibox_get_orientation,
                            (lua_class_propfunc_t) luaA_wibox_set_orientation);
    luaA_class_add_property(&wibox_class, "ontop",
                            (lua_class_propfunc_t) luaA_wibox_set_ontop,
                            (lua_class_propfunc_t) luaA_wibox_get_ontop,
                            (lua_class_propfunc_t) luaA_wibox_set_ontop);
    luaA_class_add_property(&wibox_class, "screen",
                            NULL,
                            (lua_class_propfunc_t) luaA_wibox_get_screen,
                            (lua_class_propfunc_t) luaA_wibox_set_screen);
    luaA_class_add_property(&wibox_class, "cursor",
                            (lua_class_propfunc_t) luaA_wibox_set_cursor,
                            (lua_class_propfunc_t) luaA_wibox_get_cursor,
                            (lua_class_propfunc_t) luaA_wibox_set_cursor);
    luaA_class_add_property(&wibox_class, "fg",
                            (lua_class_propfunc_t) luaA_wibox_set_fg,
                            (lua_class_propfunc_t) luaA_wibox_get_fg,
                            (lua_class_propfunc_t) luaA_wibox_set_fg);
    luaA_class_add_property(&wibox_class, "bg",
                            (lua_class_propfunc_t) luaA_wibox_set_bg,
                            (lua_class_propfunc_t) luaA_wibox_get_bg,
                            (lua_class_propfunc_t) luaA_wibox_set_bg);
    luaA_class_add_property(&wibox_class, "bg_image",
                            (lua_class_propfunc_t) luaA_wibox_set_bg_image,
                            (lua_class_propfunc_t) luaA_wibox_get_bg_image,
                            (lua_class_propfunc_t) luaA_wibox_set_bg_image);
    luaA_class_add_property(&wibox_class, "x",
                            (lua_class_propfunc_t) luaA_wibox_set_x,
                            (lua_class_propfunc_t) luaA_wibox_get_x,
                            (lua_class_propfunc_t) luaA_wibox_set_x);
    luaA_class_add_property(&wibox_class, "y",
                            (lua_class_propfunc_t) luaA_wibox_set_y,
                            (lua_class_propfunc_t) luaA_wibox_get_y,
                            (lua_class_propfunc_t) luaA_wibox_set_y);
    luaA_class_add_property(&wibox_class, "width",
                            (lua_class_propfunc_t) luaA_wibox_set_width,
                            (lua_class_propfunc_t) luaA_wibox_get_width,
                            (lua_class_propfunc_t) luaA_wibox_set_width);
    luaA_class_add_property(&wibox_class, "height",
                            (lua_class_propfunc_t) luaA_wibox_set_height,
                            (lua_class_propfunc_t) luaA_wibox_get_height,
                            (lua_class_propfunc_t) luaA_wibox_set_height);
    luaA_class_add_property(&wibox_class, "type",
                            (lua_class_propfunc_t) luaA_window_set_type,
                            (lua_class_propfunc_t) luaA_window_get_type,
                            (lua_class_propfunc_t) luaA_window_set_type);

    signal_add(&wibox_class.signals, "mouse::enter");
    signal_add(&wibox_class.signals, "mouse::leave");
    signal_add(&wibox_class.signals, "property::bg");
    signal_add(&wibox_class.signals, "property::bg_image");
    signal_add(&wibox_class.signals, "property::border_width");
    signal_add(&wibox_class.signals, "property::cursor");
    signal_add(&wibox_class.signals, "property::fg");
    signal_add(&wibox_class.signals, "property::height");
    signal_add(&wibox_class.signals, "property::ontop");
    signal_add(&wibox_class.signals, "property::orientation");
    signal_add(&wibox_class.signals, "property::screen");
    signal_add(&wibox_class.signals, "property::visible");
    signal_add(&wibox_class.signals, "property::widgets");
    signal_add(&wibox_class.signals, "property::width");
    signal_add(&wibox_class.signals, "property::x");
    signal_add(&wibox_class.signals, "property::y");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
