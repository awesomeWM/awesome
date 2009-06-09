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

#include <xcb/shape.h>

#include "screen.h"
#include "wibox.h"
#include "titlebar.h"
#include "client.h"
#include "screen.h"
#include "window.h"
#include "common/xcursor.h"
#include "common/xutil.h"

DO_LUA_TOSTRING(wibox_t, wibox, "wibox")

/** Take care of garbage collecting a wibox.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack, 0!
 */
static int
luaA_wibox_gc(lua_State *L)
{
    wibox_t *wibox = luaL_checkudata(L, 1, "wibox");
    p_delete(&wibox->cursor);
    wibox_wipe(wibox);
    widget_node_array_wipe(&wibox->widgets);
    return luaA_object_gc(L);
}

void
wibox_unref_simplified(wibox_t **item)
{
    wibox_unref(globalconf.L, *item);
}

static void
wibox_need_update(wibox_t *wibox)
{
    wibox->need_update = true;
    wibox->mouse_over = NULL;
}

static int
have_shape(void)
{
    const xcb_query_extension_reply_t *reply;

    reply = xcb_get_extension_data(globalconf.connection, &xcb_shape_id);
    if (!reply || !reply->present)
        return 0;

    /* We don't need a specific version of SHAPE, no version check required */
    return 1;
}

static void
shape_update(xcb_window_t win, xcb_shape_kind_t kind, image_t *image, int offset)
{
    xcb_pixmap_t shape;

    if(image)
        shape = image_to_1bit_pixmap(image, win);
    else
        /* Reset the shape */
        shape = XCB_NONE;

    xcb_shape_mask(globalconf.connection, XCB_SHAPE_SO_SET, kind,
                   win, offset, offset, shape);

    if (shape != XCB_NONE)
        xcb_free_pixmap(globalconf.connection, shape);
}

/** Update the window's shape.
 * \param wibox The simplw window whose shape should be updated.
 */
static void
wibox_shape_update(wibox_t *wibox)
{
    if(wibox->window == XCB_NONE)
        return;

    if(!have_shape())
    {
        static bool warned = false;
        if(!warned)
            warn("The X server doesn't have the SHAPE extension; "
                    "can't change window's shape");
        warned = true;
        return;
    }

    shape_update(wibox->window, XCB_SHAPE_SK_CLIP, wibox->shape.clip, 0);
    shape_update(wibox->window, XCB_SHAPE_SK_BOUNDING, wibox->shape.bounding, - wibox->border.width);

    wibox->need_shape_update = false;
}

static void
wibox_draw_context_update(wibox_t *w, xcb_screen_t *s)
{
    xcolor_t fg = w->ctx.fg, bg = w->ctx.bg;
    int phys_screen = w->ctx.phys_screen;

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
        draw_context_init(&w->ctx, phys_screen,
                          w->geometry.height,
                          w->geometry.width,
                          w->ctx.pixmap, &fg, &bg);
        break;
      case East:
        draw_context_init(&w->ctx, phys_screen,
                          w->geometry.width,
                          w->geometry.height,
                          w->pixmap, &fg, &bg);
        break;
    }
}

/** Initialize a wibox.
 * \param w The wibox to initialize.
 * \param phys_screen Physical screen number.
 */
void
wibox_init(wibox_t *w, int phys_screen)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);

    w->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, s->root_depth, w->window, s->root,
                      w->geometry.x, w->geometry.y,
                      w->geometry.width, w->geometry.height,
                      w->border.width, XCB_COPY_FROM_PARENT, s->root_visual,
                      XCB_CW_BACK_PIXMAP | XCB_CW_BORDER_PIXEL
                      | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                      (const uint32_t [])
                      {
                          XCB_BACK_PIXMAP_PARENT_RELATIVE,
                          w->border.color.pixel,
                          1,
                          XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                          | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
                          | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                          | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
                          | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE
                          | XCB_EVENT_MASK_PROPERTY_CHANGE
                      });

    /* Create a pixmap. */
    w->pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, s->root_depth, w->pixmap, s->root,
                      w->geometry.width, w->geometry.height);

    /* Update draw context physical screen, important for Zaphod. */
    w->ctx.phys_screen = phys_screen;
    wibox_draw_context_update(w, s);

    /* The default GC is just a newly created associated to the root window */
    w->gc = xcb_generate_id(globalconf.connection);
    xcb_create_gc(globalconf.connection, w->gc, s->root, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND,
                  (const uint32_t[]) { s->black_pixel, s->white_pixel });

    wibox_shape_update(w);
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param w The wibox to refresh.
 */
static inline void
wibox_refresh_pixmap(wibox_t *w)
{
    wibox_refresh_pixmap_partial(w, 0, 0, w->geometry.width, w->geometry.height);
}

/** Set a wibox opacity.
 * \param w The wibox the adjust the opacity of.
 * \param opacity A value between 0 and 1 which describes the opacity.
 */
static inline void
wibox_opacity_set(wibox_t *w, double opacity)
{
    w->opacity = opacity;
    if(w->window != XCB_NONE)
        window_opacity_set(w->window, opacity);
}

void
wibox_moveresize(wibox_t *w, area_t geometry)
{
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
            xcb_screen_t *s = xutil_screen_get(globalconf.connection, w->ctx.phys_screen);
            xcb_create_pixmap(globalconf.connection, s->root_depth, w->pixmap, s->root,
                              w->geometry.width, w->geometry.height);
            wibox_draw_context_update(w, s);
        }

        if(mask_vals)
            xcb_configure_window(globalconf.connection, w->window, mask_vals, moveresize_win_vals);
    }
    else
        w->geometry = geometry;

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
                  wibox->window, wibox->gc, x, y, x, y,
                  w, h);
}

/** Set a wibox border width.
 * \param w The wibox to change border width.
 * \param border_width The border width in pixel.
 */
void
wibox_border_width_set(wibox_t *w, uint32_t border_width)
{
    xcb_configure_window(globalconf.connection, w->window, XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         &border_width);
    w->border.width = border_width;
}

/** Set a wibox border color.
 * \param w The wibox to change border width.
 * \param color The border color.
 */
void
wibox_border_color_set(wibox_t *w, const xcolor_t *color)
{
    xcb_change_window_attributes(globalconf.connection, w->window,
                                 XCB_CW_BORDER_PIXEL, &color->pixel);
    w->border.color = *color;
}

/** Set wibox orientation.
 * \param w The wibox.
 * \param o The new orientation.
 */
void
wibox_orientation_set(wibox_t *w, orientation_t o)
{
    if(o != w->orientation)
    {
        xcb_screen_t *s = xutil_screen_get(globalconf.connection, w->ctx.phys_screen);
        w->orientation = o;
        /* orientation != East */
        if(w->pixmap != w->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, w->ctx.pixmap);
        wibox_draw_context_update(w, s);
    }
}

/** Set wibox cursor.
 * \param w The wibox.
 * \param c The cursor.
 */
static void
wibox_cursor_set(wibox_t *w, xcb_cursor_t c)
{
    if(w->window)
        xcb_change_window_attributes(globalconf.connection, w->window, XCB_CW_CURSOR,
                                     (const uint32_t[]) { c });
}

static void
wibox_map(wibox_t *wibox)
{
    xcb_map_window(globalconf.connection, wibox->window);
    /* We must make sure the wibox does not display garbage */
    wibox_need_update(wibox);
    /* Stack this wibox correctly */
    client_stack();
}

static void
wibox_move(wibox_t *wibox, int16_t x, int16_t y)
{
    if(wibox->window
       && (x != wibox->geometry.x || y != wibox->geometry.y))
    {
        xcb_configure_window(globalconf.connection, wibox->window,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             (const uint32_t[]) { x, y });

        wibox->screen = screen_getbycoord(wibox->screen, x, y);
    }

    wibox->geometry.x = x;
    wibox->geometry.y = y;
}

static void
wibox_resize(wibox_t *w, uint16_t width, uint16_t height)
{
    if(width <= 0 || height <= 0 || (w->geometry.width == width && w->geometry.height == height))
        return;

    w->geometry.width = width;
    w->geometry.height = height;

    if(w->window)
    {
        xcb_screen_t *s = xutil_screen_get(globalconf.connection, w->ctx.phys_screen);

        xcb_free_pixmap(globalconf.connection, w->pixmap);
        /* orientation != East */
        if(w->pixmap != w->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, w->ctx.pixmap);
        w->pixmap = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth, w->pixmap, s->root,
                          w->geometry.width, w->geometry.height);
        xcb_configure_window(globalconf.connection, w->window,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             (const uint32_t[]) { w->geometry.width, w->geometry.height });
        wibox_draw_context_update(w, s);
    }

    wibox_need_update(w);
}

/** Kick out systray windows.
 * \param phys_screen Physical screen number.
 */
static void
wibox_systray_kickout(int phys_screen)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);

    if(globalconf.screens.tab[phys_screen].systray.parent != s->root)
    {
        /* Who! Check that we're not deleting a wibox with a systray, because it
         * may be its parent. If so, we reparent to root before, otherwise it will
         * hurt very much. */
        xcb_reparent_window(globalconf.connection,
                            globalconf.screens.tab[phys_screen].systray.window,
                            s->root, -512, -512);

        globalconf.screens.tab[phys_screen].systray.parent = s->root;
    }
}

static void
wibox_systray_refresh(wibox_t *wibox)
{
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
            int phys_screen = wibox->ctx.phys_screen;

            if(wibox->isvisible
               && systray->widget->isvisible
               && systray->geometry.width)
            {
                /* Set background of the systray window. */
                xcb_change_window_attributes(globalconf.connection,
                                             globalconf.screens.tab[phys_screen].systray.window,
                                             XCB_CW_BACK_PIXEL, config_back);
                /* Map it. */
                xcb_map_window(globalconf.connection, globalconf.screens.tab[phys_screen].systray.window);
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
                if(globalconf.screens.tab[phys_screen].systray.parent != wibox->window)
                {
                    xcb_reparent_window(globalconf.connection,
                                        globalconf.screens.tab[phys_screen].systray.window,
                                        wibox->window,
                                        config_win_vals[0], config_win_vals[1]);
                    globalconf.screens.tab[phys_screen].systray.parent = wibox->window;
                }
                xcb_configure_window(globalconf.connection,
                                     globalconf.screens.tab[phys_screen].systray.window,
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
                return wibox_systray_kickout(phys_screen);

            switch(wibox->orientation)
            {
              case North:
                config_win_vals[1] = systray->geometry.width - config_win_vals[3];
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
                    if(em->phys_screen == phys_screen)
                    {
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
                }
                break;
              case South:
                config_win_vals[1] = 0;
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
                    if(em->phys_screen == phys_screen)
                    {
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
                }
                break;
              case East:
                config_win_vals[1] = 0;
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
                    if(em->phys_screen == phys_screen)
                    {
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

    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        if(c->titlebar && c->titlebar->window == win)
            return c->titlebar;
    }

    return NULL;
}

/** Draw a wibox.
 * \param wibox The wibox to draw.
 */
static void
wibox_draw(wibox_t *wibox)
{
    if(wibox->isvisible)
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
        if((*w)->need_shape_update)
            wibox_shape_update(*w);
        if((*w)->need_update)
            wibox_draw(*w);
    }

    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        if(c->titlebar && c->titlebar->need_update)
            wibox_draw(c->titlebar);
    }
}

/** Set a wibox visible or not.
 * \param wibox The wibox.
 * \param v The visible value.
 */
static void
wibox_setvisible(wibox_t *wibox, bool v)
{
    if(v != wibox->isvisible)
    {
        wibox->isvisible = v;
        wibox->mouse_over = NULL;

        if(wibox->screen)
        {
            if(wibox->isvisible)
                wibox_map(wibox);
            else
                xcb_unmap_window(globalconf.connection, wibox->window);

            /* kick out systray if needed */
            wibox_systray_refresh(wibox);
        }

        hook_property(wibox, wibox, "visible");
    }
}

/** Destroy all X resources of a wibox.
 * \param w The wibox to wipe.
 */
void
wibox_wipe(wibox_t *w)
{
    if(w->window)
    {
        xcb_destroy_window(globalconf.connection, w->window);
        w->window = XCB_NONE;
    }
    if(w->pixmap)
    {
        xcb_free_pixmap(globalconf.connection, w->pixmap);
        w->pixmap = XCB_NONE;
    }
    if(w->gc)
    {
        xcb_free_gc(globalconf.connection, w->gc);
        w->gc = XCB_NONE;
    }
    draw_context_wipe(&w->ctx);
}

/** Remove a wibox from a screen.
 * \param wibox Wibox to detach from screen.
 */
static void
wibox_detach(wibox_t *wibox)
{
    if(wibox->screen)
    {
        bool v;

        /* save visible state */
        v = wibox->isvisible;
        wibox->isvisible = false;
        wibox_systray_refresh(wibox);
        /* restore visibility */
        wibox->isvisible = v;

        wibox->mouse_over = NULL;

        wibox_wipe(wibox);

        foreach(item, globalconf.wiboxes)
            if(*item == wibox)
            {
                wibox_array_remove(&globalconf.wiboxes, item);
                break;
            }

        hook_property(wibox, wibox, "screen");

        wibox->screen = NULL;

        wibox_unref(globalconf.L, wibox);
    }
}

/** Attach a wibox that is on top of the stack.
 * \param s The screen to attach the wibox to.
 */
static void
wibox_attach(screen_t *s)
{
    int phys_screen = screen_virttophys(screen_array_indexof(&globalconf.screens, s));

    wibox_t *wibox = wibox_ref(globalconf.L, -1);

    wibox_detach(wibox);

    /* Set the wibox screen */
    wibox->screen = s;

    /* Check that the wibox coordinates matches the screen. */
    screen_t *cscreen =
        screen_getbycoord(wibox->screen, wibox->geometry.x, wibox->geometry.y);

    /* If it does not match, move it to the screen coordinates */
    if(cscreen != wibox->screen)
        wibox_move(wibox, s->geometry.x, s->geometry.y);

    wibox_array_append(&globalconf.wiboxes, wibox);

    wibox_init(wibox, phys_screen);

    wibox_cursor_set(wibox,
                     xcursor_new(globalconf.connection, xcursor_font_fromstr(wibox->cursor)));

    wibox_opacity_set(wibox, wibox->opacity);

    if(wibox->isvisible)
        wibox_map(wibox);
    else
        wibox_need_update(wibox);

    hook_property(wibox, wibox, "screen");
}

/** Create a new wibox.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with optionaly defined values:
 * fg, bg, border_width, border_color, ontop, width and height.
 * \lreturn A brand new wibox.
 */
static int
luaA_wibox_new(lua_State *L)
{
    wibox_t *w;
    const char *buf;
    size_t len;
    xcolor_init_request_t reqs[3];
    int i, reqs_nbr = -1;

    luaA_checktable(L, 2);

    w = wibox_new(L);

    w->ctx.fg = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "fg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->ctx.fg, buf, len);

    w->ctx.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->ctx.bg, buf, len);

    w->border.color = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "border_color", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->border.color, buf, len);

    w->ontop = luaA_getopt_boolean(L, 2, "ontop", false);

    w->opacity = -1;
    w->border.width = luaA_getopt_number(L, 2, "border_width", 0);
    w->geometry.x = luaA_getopt_number(L, 2, "x", 0);
    w->geometry.y = luaA_getopt_number(L, 2, "y", 0);
    w->geometry.width = luaA_getopt_number(L, 2, "width", 100);
    w->geometry.height = luaA_getopt_number(L, 2, "height", globalconf.font->height * 1.5);

    w->isvisible = true;
    w->cursor = a_strdup("left_ptr");

    for(i = 0; i <= reqs_nbr; i++)
        xcolor_init_reply(reqs[i]);

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
        wibox_push(L, wibox);
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

    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        if(c->titlebar && luaA_wibox_hasitem(L, c->titlebar, item))
        {
            /* update wibox */
            wibox_need_update(c->titlebar);
            lua_pop(L, 1); /* remove widgets table */
        }
    }
}

/** Wibox object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield screen Screen number.
 * \lfield client The client attached to (titlebar).
 * \lfield border_width Border width.
 * \lfield border_color Border color.
 * \lfield align The alignment (titlebar).
 * \lfield fg Foreground color.
 * \lfield bg Background color.
 * \lfield bg_image Background image.
 * \lfield position The position (titlebar).
 * \lfield ontop On top of other windows.
 * \lfield cursor The mouse cursor.
 * \lfield visible Visibility.
 * \lfield orientation The drawing orientation: east, north or south.
 * \lfield widgets An array with all widgets drawn on this wibox.
 * \lfield opacity The opacity of the wibox, between 0 and 1.
 * \lfield mouse_enter A function to execute when the mouse enter the widget.
 * \lfield mouse_leave A function to execute when the mouse leave the widget.
 * \lfield shape_clip Image describing the window's content shape.
 * \lfield shape_bounding Image describing the window's border shape.
 */
static int
luaA_wibox_index(lua_State *L)
{
    size_t len;
    wibox_t *wibox = luaL_checkudata(L, 1, "wibox");
    const char *attr = luaL_checklstring(L, 2, &len);

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(attr, len))
    {
      case A_TK_VISIBLE:
        lua_pushboolean(L, wibox->isvisible);
        break;
      case A_TK_CLIENT:
        return client_push(L, client_getbytitlebar(wibox));
      case A_TK_SCREEN:
        if(!wibox->screen)
            return 0;
        lua_pushnumber(L, screen_array_indexof(&globalconf.screens, wibox->screen) + 1);
        break;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, wibox->border.width);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushxcolor(L, &wibox->border.color);
        break;
      case A_TK_ALIGN:
        if(wibox->type == WIBOX_TYPE_NORMAL)
            luaA_deprecate(L, "awful.wibox.align");
        lua_pushstring(L, draw_align_tostr(wibox->align));
        break;
      case A_TK_FG:
        luaA_pushxcolor(L, &wibox->ctx.fg);
        break;
      case A_TK_BG:
        luaA_pushxcolor(L, &wibox->ctx.bg);
        break;
      case A_TK_BG_IMAGE:
        luaA_object_push_item(L, 1, wibox->bg_image);
        break;
      case A_TK_POSITION:
        if(wibox->type == WIBOX_TYPE_NORMAL)
            luaA_deprecate(L, "awful.wibox.attach");
        lua_pushstring(L, position_tostr(wibox->position));
        break;
      case A_TK_ONTOP:
        lua_pushboolean(L, wibox->ontop);
        break;
      case A_TK_ORIENTATION:
        lua_pushstring(L, orientation_tostr(wibox->orientation));
        break;
      case A_TK_WIDGETS:
        return luaA_object_push_item(L, 1, wibox->widgets_table);
      case A_TK_CURSOR:
        lua_pushstring(L, wibox->cursor);
        break;
      case A_TK_OPACITY:
        if (wibox->opacity >= 0)
            lua_pushnumber(L, wibox->opacity);
        else
            return 0;
        break;
      case A_TK_MOUSE_ENTER:
        return luaA_object_push_item(L, 1, wibox->mouse_enter);
      case A_TK_MOUSE_LEAVE:
        return luaA_object_push_item(L, 1, wibox->mouse_leave);
      case A_TK_SHAPE_BOUNDING:
        return luaA_object_push_item(L, 1, wibox->shape.bounding);
      case A_TK_SHAPE_CLIP:
        return luaA_object_push_item(L, 1, wibox->shape.clip);
      case A_TK_BUTTONS:
        return luaA_object_push_item(L, 1, wibox->buttons);
      default:
        return 0;
    }

    return 1;
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
    wibox_t *wibox = luaL_checkudata(L, 1, "wibox");

    if(lua_gettop(L) == 2)
    {
        area_t wingeom;

        luaA_checktable(L, 2);
        wingeom.x = luaA_getopt_number(L, 2, "x", wibox->geometry.x);
        wingeom.y = luaA_getopt_number(L, 2, "y", wibox->geometry.y);
        wingeom.width = luaA_getopt_number(L, 2, "width", wibox->geometry.width);
        wingeom.height = luaA_getopt_number(L, 2, "height", wibox->geometry.height);

        switch(wibox->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            wibox_resize(wibox, wingeom.width, wingeom.height);
            break;
          case WIBOX_TYPE_NORMAL:
            wibox_moveresize(wibox, wingeom);
            break;
        }
    }

    return luaA_pusharea(L, wibox->geometry);
}

/** Wibox newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_newindex(lua_State *L)
{
    size_t len;
    wibox_t *wibox = luaL_checkudata(L, 1, "wibox");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    awesome_token_t tok;

    switch((tok = a_tokenize(attr, len)))
    {
        bool b;

      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&wibox->ctx.fg, buf, len)))
                wibox->need_update = true;
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&wibox->ctx.bg, buf, len)))
                wibox->need_update = true;
        break;
      case A_TK_BG_IMAGE:
        luaA_object_unref_item(L, 1, wibox->bg_image);
        wibox->bg_image = luaA_object_ref_item(L, 1, 3);
        wibox->need_update = true;
        break;
      case A_TK_ALIGN:
        buf = luaL_checklstring(L, 3, &len);
        wibox->align = draw_align_fromstr(buf, len);
        switch(wibox->type)
        {
          case WIBOX_TYPE_NORMAL:
            luaA_deprecate(L, "awful.wibox.align");
            break;
          case WIBOX_TYPE_TITLEBAR:
            titlebar_update_geometry(client_getbytitlebar(wibox));
            break;
        }
        break;
      case A_TK_POSITION:
        switch(wibox->type)
        {
          case WIBOX_TYPE_NORMAL:
            if((buf = luaL_checklstring(L, 3, &len)))
            {
                luaA_deprecate(L, "awful.wibox.attach");
                wibox->position = position_fromstr(buf, len);
            }
            break;
          case WIBOX_TYPE_TITLEBAR:
            return luaA_titlebar_newindex(L, wibox, tok);
        }
        break;
      case A_TK_CLIENT:
        /* first detach */
        if(lua_isnil(L, 3))
            titlebar_client_detach(client_getbytitlebar(wibox));
        else
        {
            client_t *c = luaA_client_checkudata(L, -1);
            lua_pushvalue(L, 1);
            titlebar_client_attach(c);
        }
        break;
      case A_TK_CURSOR:
        if((buf = luaL_checkstring(L, 3)))
        {
            uint16_t cursor_font = xcursor_font_fromstr(buf);
            if(cursor_font)
            {
                xcb_cursor_t cursor = xcursor_new(globalconf.connection, cursor_font);
                p_delete(&wibox->cursor);
                wibox->cursor = a_strdup(buf);
                wibox_cursor_set(wibox, cursor);
            }
        }
        break;
      case A_TK_SCREEN:
        if(lua_isnil(L, 3))
        {
            wibox_detach(wibox);
            titlebar_client_detach(client_getbytitlebar(wibox));
        }
        else
        {
            int screen = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(screen);
            if(!wibox->screen || screen != screen_array_indexof(&globalconf.screens, wibox->screen))
            {
                titlebar_client_detach(client_getbytitlebar(wibox));
                lua_pushvalue(L, 1);
                wibox_attach(&globalconf.screens.tab[screen]);
            }
        }
        break;
      case A_TK_ONTOP:
        b = luaA_checkboolean(L, 3);
        if(b != wibox->ontop)
        {
            wibox->ontop = b;
            client_stack();
        }
        break;
      case A_TK_ORIENTATION:
        if((buf = luaL_checklstring(L, 3, &len)))
        {
            wibox_orientation_set(wibox, orientation_fromstr(buf, len));
            wibox_need_update(wibox);
        }
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&wibox->border.color, buf, len)))
                if(wibox->window)
                    wibox_border_color_set(wibox, &wibox->border.color);
        break;
      case A_TK_VISIBLE:
        b = luaA_checkboolean(L, 3);
        if(b != wibox->isvisible)
            switch(wibox->type)
            {
              case WIBOX_TYPE_NORMAL:
                wibox_setvisible(wibox, b);
                break;
              case WIBOX_TYPE_TITLEBAR:
                titlebar_set_visible(wibox, b);
                break;
            }
        break;
      case A_TK_WIDGETS:
        if(luaA_isloop(L, 3))
        {
            luaA_warn(L, "table is looping, cannot use this as widget table");
            return 0;
        }
        /* duplicate table because next function will eat it */
        lua_pushvalue(L, 3);
        /* register object */
        wibox->widgets_table = luaA_object_ref_item(L, 1, -1);
        wibox_need_update(wibox);
        luaA_table2wtable(L);
        break;
      case A_TK_OPACITY:
        if(lua_isnil(L, 3))
            wibox_opacity_set(wibox, -1);
        else
        {
            double d = luaL_checknumber(L, 3);
            if(d >= 0 && d <= 1)
                wibox_opacity_set(wibox, d);
        }
        break;
      case A_TK_MOUSE_ENTER:
        luaA_checkfunction(L, 3);
        luaA_object_unref_item(L, 1, wibox->mouse_enter);
        wibox->mouse_enter = luaA_object_ref_item(L, 1, 3);
        return 0;
      case A_TK_MOUSE_LEAVE:
        luaA_checkfunction(L, 3);
        luaA_object_unref_item(L, 1, wibox->mouse_leave);
        wibox->mouse_leave = luaA_object_ref_item(L, 1, 3);
        return 0;
      case A_TK_SHAPE_BOUNDING:
        luaA_object_unref_item(L, 1, wibox->shape.bounding);
        wibox->shape.bounding = luaA_object_ref_item(L, 1, 3);
        wibox->need_shape_update = true;
        break;
      case A_TK_SHAPE_CLIP:
        luaA_object_unref_item(L, 1, wibox->shape.clip);
        wibox->shape.clip = luaA_object_ref_item(L, 1, 3);
        wibox->need_shape_update = true;
      case A_TK_BUTTONS:
        luaA_object_unref_item(L, 1, wibox->buttons);
        wibox->buttons = luaA_object_ref_item(L, 1, 3);
        break;
      default:
        switch(wibox->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            return luaA_titlebar_newindex(L, wibox, tok);
          case WIBOX_TYPE_NORMAL:
            break;
        }
    }

    return 0;
}

const struct luaL_reg awesome_wibox_methods[] =
{
    LUA_CLASS_METHODS(wibox)
    { "__call", luaA_wibox_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_wibox_meta[] =
{
    LUA_OBJECT_META
    { "geometry", luaA_wibox_geometry },
    { "__index", luaA_wibox_index },
    { "__newindex", luaA_wibox_newindex },
    { "__gc", luaA_wibox_gc },
    { "__tostring", luaA_wibox_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
