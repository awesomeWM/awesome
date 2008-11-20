/*
 * wibox.c - wibox functions
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

#include "screen.h"
#include "wibox.h"
#include "titlebar.h"
#include "client.h"
#include "ewmh.h"
#include "common/xcursor.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, wibox_t, wibox, "wibox", wibox_ref)
DO_LUA_GC(wibox_t, wibox, "wibox", wibox_unref)
DO_LUA_EQ(wibox_t, wibox, "wibox")

static void
wibox_move(wibox_t *wibox, int16_t x, int16_t y)
{
    if(wibox->sw.window)
        simplewindow_move(&wibox->sw, x, y);
    else
    {
        wibox->sw.geometry.x = x;
        wibox->sw.geometry.y = y;
    }
}

static void
wibox_resize(wibox_t *wibox, uint16_t width, uint16_t height)
{
    if(wibox->sw.window)
        simplewindow_resize(&wibox->sw, width, height);
    else
    {
        wibox->sw.geometry.width = width;
        wibox->sw.geometry.height = height;
    }
    wibox->need_update = true;
}

static void
wibox_setposition(wibox_t *wibox, position_t p)
{
    if(p != wibox->position)
    {
        switch((wibox->position = p))
        {
          case Bottom:
          case Top:
          case Floating:
            simplewindow_orientation_set(&wibox->sw, East);
            break;
          case Left:
            simplewindow_orientation_set(&wibox->sw, North);
            break;
          case Right:
            simplewindow_orientation_set(&wibox->sw, South);
            break;
        }
        /* reset width/height to 0 */
        if(wibox->position != Floating)
            wibox->sw.geometry.width = wibox->sw.geometry.height = 0;

        /* recompute position */
        wibox_position_update(wibox);

        /* reset all wibox position */
        wibox_array_t *w = &globalconf.screens[wibox->screen].wiboxes;
        for(int i = 0; i < w->len; i++)
            wibox_position_update(w->tab[i]);

        ewmh_update_workarea(screen_virttophys(wibox->screen));

        wibox->need_update = true;
    }
}

/** Kick out systray windows.
 * \param phys_screen Physical screen number.
 */
static void
wibox_systray_kickout(int phys_screen)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);

    /* Who! Check that we're not deleting a wibox with a systray, because it
     * may be its parent. If so, we reparent to root before, otherwise it will
     * hurt very much. */
    xcb_reparent_window(globalconf.connection,
                        globalconf.screens[phys_screen].systray.window,
                        s->root, -512, -512);

    globalconf.screens[phys_screen].systray.parent = s->root;
}

static void
wibox_systray_refresh(wibox_t *wibox)
{
    if(wibox->screen == SCREEN_UNDEF)
        return;

    for(int i = 0; i < wibox->widgets.len; i++)
    {
        widget_node_t *systray = &wibox->widgets.tab[i];
        if(systray->widget->type == widget_systray)
        {
            uint32_t config_back[] = { wibox->sw.ctx.bg.pixel };
            uint32_t config_win_vals[4];
            uint32_t config_win_vals_off[2] = { -512, -512 };
            xembed_window_t *em;
            position_t pos;
            int phys_screen = wibox->sw.ctx.phys_screen;

            if(wibox->isvisible
               && systray->widget->isvisible
               && systray->geometry.width)
            {
                pos = wibox->position;

                /* Set background of the systray window. */
                xcb_change_window_attributes(globalconf.connection,
                                             globalconf.screens[phys_screen].systray.window,
                                             XCB_CW_BACK_PIXEL, config_back);
                /* Map it. */
                xcb_map_window(globalconf.connection, globalconf.screens[phys_screen].systray.window);
                /* Move it. */
                switch(wibox->position)
                {
                  case Left:
                    config_win_vals[0] = systray->geometry.y;
                    config_win_vals[1] = wibox->sw.geometry.height - systray->geometry.x - systray->geometry.width;
                    config_win_vals[2] = systray->geometry.height;
                    config_win_vals[3] = systray->geometry.width;
                    break;
                  case Right:
                    config_win_vals[0] = systray->geometry.y;
                    config_win_vals[1] = systray->geometry.x;
                    config_win_vals[2] = systray->geometry.height;
                    config_win_vals[3] = systray->geometry.width;
                    break;
                  default:
                    config_win_vals[0] = systray->geometry.x;
                    config_win_vals[1] = systray->geometry.y;
                    config_win_vals[2] = systray->geometry.width;
                    config_win_vals[3] = systray->geometry.height;
                    break;
                }
                /* reparent */
                if(globalconf.screens[phys_screen].systray.parent != wibox->sw.window)
                {
                    xcb_reparent_window(globalconf.connection,
                                        globalconf.screens[phys_screen].systray.window,
                                        wibox->sw.window,
                                        config_win_vals[0], config_win_vals[1]);
                    globalconf.screens[phys_screen].systray.parent = wibox->sw.window;
                }
                xcb_configure_window(globalconf.connection,
                                     globalconf.screens[phys_screen].systray.window,
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

            switch(pos)
            {
              case Left:
                config_win_vals[1] = systray->geometry.width - config_win_vals[3];
                for(em = globalconf.embedded; em; em = em->next)
                    if(em->phys_screen == phys_screen)
                    {
                        if(config_win_vals[1] - config_win_vals[2] >= (uint32_t) wibox->sw.geometry.y)
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
              case Right:
                config_win_vals[1] = 0;
                for(em = globalconf.embedded; em; em = em->next)
                    if(em->phys_screen == phys_screen)
                    {
                        if(config_win_vals[1] + config_win_vals[3] <= (uint32_t) wibox->sw.geometry.y + wibox->sw.geometry.width)
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
              case Floating:
              case Top:
              case Bottom:
                config_win_vals[1] = 0;
                for(em = globalconf.embedded; em; em = em->next)
                    if(em->phys_screen == phys_screen)
                    {
                        /* if(x + width < systray.x + systray.width) */
                        if(config_win_vals[0] + config_win_vals[2] <= (uint32_t) AREA_RIGHT(systray->geometry) + wibox->sw.geometry.x)
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

/** Update the wibox position. It deletes every wibox resources and
 * create them back.
 * \param wibox The wibox.
 */
void
wibox_position_update(wibox_t *wibox)
{
    area_t area, wingeom = wibox->sw.geometry;
    bool ignore = false;

    globalconf.screens[wibox->screen].need_arrange = true;

    area = screen_area_get(wibox->screen, NULL,
                           &globalconf.screens[wibox->screen].padding, true);

    /* Top and Bottom wibox_t have prio */
    if(wibox->position != Floating)
        for(int i = 0; i < globalconf.screens[wibox->screen].wiboxes.len; i++)
        {
            wibox_t *w = globalconf.screens[wibox->screen].wiboxes.tab[i];
            /* Ignore every wibox after me that is in the same position */
            if(wibox == w)
            {
                ignore = true;
                continue;
            }
            else if((ignore && wibox->position == w->position) || !w->isvisible)
                continue;
            switch(w->position)
            {
              case Floating:
                break;
              case Left:
                switch(wibox->position)
                {
                  case Left:
                    area.x += wibox->sw.geometry.height;
                    break;
                  default:
                    break;
                }
                break;
              case Right:
                switch(wibox->position)
                {
                  case Right:
                    area.x -= wibox->sw.geometry.height;
                    break;
                  default:
                    break;
                }
                break;
              case Top:
                switch(wibox->position)
                {
                  case Top:
                    area.y += w->sw.geometry.height;
                    break;
                  case Left:
                  case Right:
                    area.height -= w->sw.geometry.height;
                    area.y += w->sw.geometry.height;
                    break;
                  default:
                    break;
                }
                break;
              case Bottom:
                switch(wibox->position)
                {
                  case Bottom:
                    area.y -= w->sw.geometry.height;
                    break;
                  case Left:
                  case Right:
                    area.height -= w->sw.geometry.height;
                    break;
                  default:
                    break;
                }
                break;
            }
        }

    switch(wibox->position)
    {
      case Right:
        wingeom.height = wibox->sw.geometry.height > 0 ?
            wibox->sw.geometry.height : area.height - 2 * wibox->sw.border.width;
        wingeom.width = wibox->sw.geometry.width > 0 ? wibox->sw.geometry.width : 1.5 * globalconf.font->height;
        wingeom.x = area.x + area.width - wingeom.width - 2 * wibox->sw.border.width;
        switch(wibox->align)
        {
          default:
            wingeom.y = area.y;
            break;
          case AlignRight:
            wingeom.y = area.y + area.height - wingeom.height;
            break;
          case AlignCenter:
            wingeom.y = (area.y + area.height - wingeom.height) / 2;
            break;
        }
        break;
      case Left:
        wingeom.height = wibox->sw.geometry.height > 0 ?
            wibox->sw.geometry.height : area.height - 2 * wibox->sw.border.width;
        wingeom.width = wibox->sw.geometry.width > 0 ? wibox->sw.geometry.width : 1.5 * globalconf.font->height;
        wingeom.x = area.x;
        switch(wibox->align)
        {
          default:
            wingeom.y = (area.y + area.height) - wingeom.height - 2 * wibox->sw.border.width;
            break;
          case AlignRight:
            wingeom.y = area.y;
            break;
          case AlignCenter:
            wingeom.y = (area.y + area.height - wingeom.height) / 2;
        }
        break;
      case Bottom:
        wingeom.height = wibox->sw.geometry.height > 0 ? wibox->sw.geometry.height : 1.5 * globalconf.font->height;
        wingeom.width = wibox->sw.geometry.width > 0 ?
            wibox->sw.geometry.width : area.width - 2 * wibox->sw.border.width;
        wingeom.y = (area.y + area.height) - wingeom.height - 2 * wibox->sw.border.width;
        wingeom.x = area.x;
        switch(wibox->align)
        {
          default:
            break;
          case AlignRight:
            wingeom.x += area.width - wingeom.width;
            break;
          case AlignCenter:
            wingeom.x += (area.width - wingeom.width) / 2;
            break;
        }
        break;
      case Top:
        wingeom.height = wibox->sw.geometry.height > 0 ? wibox->sw.geometry.height : 1.5 * globalconf.font->height;
        wingeom.width = wibox->sw.geometry.width > 0 ?
            wibox->sw.geometry.width : area.width - 2 * wibox->sw.border.width;
        wingeom.x = area.x;
        wingeom.y = area.y;
        switch(wibox->align)
        {
          default:
            break;
          case AlignRight:
            wingeom.x += area.width - wingeom.width;
            break;
          case AlignCenter:
            wingeom.x += (area.width - wingeom.width) / 2;
            break;
        }
        break;
      case Floating:
        wingeom.width = MAX(1, wibox->sw.geometry.width);
        wingeom.height = MAX(1, wibox->sw.geometry.height);
        break;
    }

    /* same window size and position ? */
    if(wingeom.width != wibox->sw.geometry.width
       || wingeom.height != wibox->sw.geometry.height)
        wibox_resize(wibox, wingeom.width, wingeom.height);

    if(wingeom.x != wibox->sw.geometry.x
        || wingeom.y != wibox->sw.geometry.y)
        wibox_move(wibox, wingeom.x, wingeom.y);
}

/** Delete a wibox.
 * \param wibox wibox to delete.
 */
void
wibox_delete(wibox_t **wibox)
{
    simplewindow_wipe(&(*wibox)->sw);
    luaL_unref(globalconf.L, LUA_REGISTRYINDEX, (*wibox)->widgets_table);
    widget_node_array_wipe(&(*wibox)->widgets);
    p_delete(wibox);
}

/** Get a wibox by its window.
 * \param w The window id.
 * \return A wibox if found, NULL otherwise.
 */
wibox_t *
wibox_getbywin(xcb_window_t w)
{
    for(int screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].wiboxes.len; i++)
        {
            wibox_t *s = globalconf.screens[screen].wiboxes.tab[i];
            if(s->sw.window == w)
                return s;
        }

    for(client_t *c = globalconf.clients; c; c = c->next)
        if(c->titlebar && c->titlebar->sw.window == w)
            return c->titlebar;

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
        widget_render(&wibox->widgets, &wibox->sw.ctx, wibox->sw.gc,
                      wibox->sw.pixmap,
                      wibox->screen, wibox->sw.orientation,
                      wibox->sw.geometry.x, wibox->sw.geometry.y,
                      wibox);
        simplewindow_refresh_pixmap(&wibox->sw);

        wibox->need_update = false;
    }

    wibox_systray_refresh(wibox);
}

/** Refresh all wiboxes.
 */
void
wibox_refresh(void)
{
    for(int screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].wiboxes.len; i++)
        {
            wibox_t *s = globalconf.screens[screen].wiboxes.tab[i];
            if(s->need_update)
                wibox_draw(s);
        }

    for(client_t *c = globalconf.clients; c; c = c->next)
        if(c->titlebar && c->titlebar->need_update)
            wibox_draw(c->titlebar);
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

        if(wibox->screen != SCREEN_UNDEF)
        {
            if(wibox->isvisible)
            {
                xcb_map_window(globalconf.connection, wibox->sw.window);
                simplewindow_refresh_pixmap(&wibox->sw);
                /* stack correctly the wibox */
                client_stack();
            }
            else
                xcb_unmap_window(globalconf.connection, wibox->sw.window);

            /* kick out systray if needed */
            wibox_systray_refresh(wibox);

            /* All the other wibox and ourselves need to be repositioned */
            wibox_array_t *w = &globalconf.screens[wibox->screen].wiboxes;
            for(int i = 0; i < w->len; i++)
                wibox_position_update(w->tab[i]);
        }
    }
}

/** Remove a wibox from a screen.
 * \param wibox Wibox to detach from screen.
 */
void
wibox_detach(wibox_t *wibox)
{
    if(wibox->screen != SCREEN_UNDEF)
    {
        bool v;

        /* save visible state */
        v = wibox->isvisible;
        wibox->isvisible = false;
        wibox_systray_refresh(wibox);
        wibox_position_update(wibox);
        /* restore position */
        wibox->isvisible = v;

        simplewindow_wipe(&wibox->sw);

        for(int i = 0; i < globalconf.screens[wibox->screen].wiboxes.len; i++)
            if(globalconf.screens[wibox->screen].wiboxes.tab[i] == wibox)
            {
                wibox_array_take(&globalconf.screens[wibox->screen].wiboxes, i);
                break;
            }
        globalconf.screens[wibox->screen].need_arrange = true;
        wibox->screen = SCREEN_UNDEF;
        wibox_unref(&wibox);
    }
}

/** Attach a wibox.
 * \param wibox The wibox to attach.
 * \param s The screen to attach the wibox to.
 */
void
wibox_attach(wibox_t *wibox, screen_t *s)
{
    int phys_screen = screen_virttophys(s->index);

    wibox_detach(wibox);

    wibox->screen = s->index;

    wibox_array_append(&s->wiboxes, wibox_ref(&wibox));

    /* compute x/y/width/height if not set */
    wibox_position_update(wibox);

    simplewindow_init(&wibox->sw, phys_screen,
                      wibox->sw.geometry,
                      wibox->sw.border.width,
                      wibox->sw.orientation,
                      &wibox->sw.ctx.fg, &wibox->sw.ctx.bg);

    simplewindow_border_color_set(&wibox->sw, &wibox->sw.border.color);

    simplewindow_cursor_set(&wibox->sw,
                            xcursor_new(globalconf.connection, xcursor_font_fromstr(wibox->cursor)));

    /* All the other wibox and ourselves need to be repositioned */
    for(int i = 0; i < s->wiboxes.len; i++)
        wibox_position_update(s->wiboxes.tab[i]);

    ewmh_update_workarea(screen_virttophys(s->index));

    if(wibox->isvisible)
    {
        /* draw it right now once to avoid garbage shown */
        wibox_draw(wibox);
        xcb_map_window(globalconf.connection, wibox->sw.window);
        simplewindow_refresh_pixmap(&wibox->sw);
        /* stack correctly the wibox */
        client_stack();
    }
    else
        wibox->need_update = true;
}

/** Create a new wibox.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with optionaly defined values:
 * position, align, fg, bg, border_width, border_color, width and height.
 * \lreturn A brand new wibox.
 */
int
luaA_wibox_new(lua_State *L)
{
    wibox_t *w;
    const char *buf;
    size_t len;
    xcolor_init_request_t reqs[3];
    int8_t i, reqs_nbr = -1;

    luaA_checktable(L, 2);

    w = p_new(wibox_t, 1);
    w->widgets_table = LUA_REFNIL;

    w->sw.ctx.fg = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "fg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->sw.ctx.fg, buf, len);

    w->sw.ctx.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->sw.ctx.bg, buf, len);

    w->sw.border.color = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "border_color", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->sw.border.color, buf, len);

    buf = luaA_getopt_lstring(L, 2, "align", "left", &len);
    w->align = draw_align_fromstr(buf, len);

    w->sw.border.width = luaA_getopt_number(L, 2, "border_width", 0);

    buf = luaA_getopt_lstring(L, 2, "position", "top", &len);

    switch((w->position = position_fromstr(buf, len)))
    {
      case Bottom:
      case Top:
      case Floating:
        simplewindow_orientation_set(&w->sw, East);
        break;
      case Left:
        simplewindow_orientation_set(&w->sw, North);
        break;
      case Right:
        simplewindow_orientation_set(&w->sw, South);
        break;
    }

    /* recompute position */
    wibox_position_update(w);

    w->sw.geometry.width = luaA_getopt_number(L, 2, "width", 0);
    w->sw.geometry.height = luaA_getopt_number(L, 2, "height", 0);

    w->screen = SCREEN_UNDEF;
    w->isvisible = true;
    w->cursor = a_strdup("left_ptr");

    for(i = 0; i <= reqs_nbr; i++)
        xcolor_init_reply(reqs[i]);

    return luaA_wibox_userdata_new(L, w);
}

/** Rebuild wibox widgets list.
 * \param L The Lua VM state.
 * \param wibox The wibox.
 */
static void
wibox_widgets_table_build(lua_State *L, wibox_t *wibox)
{
    widget_node_array_wipe(&wibox->widgets);
    widget_node_array_init(&wibox->widgets);
    luaA_table2widgets(L, &wibox->widgets);
    wibox->mouse_over = NULL;
    wibox->need_update = true;
}

/** Check if a wibox widget table has an item.
 * \param L The Lua VM state.
 * \param wibox The wibox.
 * \param item The item to look for.
 */
static bool
luaA_wibox_hasitem(lua_State *L, wibox_t *wibox, const void *item)
{
    if(wibox->widgets_table != LUA_REFNIL)
    {
        lua_rawgeti(globalconf.L, LUA_REGISTRYINDEX, wibox->widgets_table);
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
    for(int screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].wiboxes.len; i++)
        {
            wibox_t *wibox = globalconf.screens[screen].wiboxes.tab[i];
            if(luaA_wibox_hasitem(L, wibox, item))
            {
                /* recompute widget node list */
                wibox_widgets_table_build(L, wibox);
                lua_pop(L, 1); /* remove widgets table */
            }

        }

    for(client_t *c = globalconf.clients; c; c = c->next)
        if(c->titlebar && luaA_wibox_hasitem(L, c->titlebar, item))
        {
            /* recompute widget node list */
            wibox_widgets_table_build(L, c->titlebar);
            lua_pop(L, 1); /* remove widgets table */
        }
}

/** Wibox object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield screen Screen number.
 * \lfield client The client attached to this titlebar.
 * \lfield border_width Border width.
 * \lfield border_color Border color.
 * \lfield align The alignment.
 * \lfield fg Foreground color.
 * \lfield bg Background color.
 * \lfield position The position.
 * \lfield ontop On top of other windows.
 * \lfield cursor The mouse cursor.
 */
static int
luaA_wibox_index(lua_State *L)
{
    size_t len;
    wibox_t **wibox = luaA_checkudata(L, 1, "wibox");
    const char *attr = luaL_checklstring(L, 2, &len);

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(attr, len))
    {
        client_t *c;

      case A_TK_VISIBLE:
        lua_pushboolean(L, (*wibox)->isvisible);
        break;
      case A_TK_CLIENT:
        if((c = client_getbytitlebar(*wibox)))
            return luaA_client_userdata_new(L, c);
        else
            return 0;
      case A_TK_SCREEN:
        if((*wibox)->screen == SCREEN_UNDEF)
            return 0;
        lua_pushnumber(L, (*wibox)->screen + 1);
        break;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, (*wibox)->sw.border.width);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushcolor(L, &(*wibox)->sw.border.color);
        break;
      case A_TK_ALIGN:
        lua_pushstring(L, draw_align_tostr((*wibox)->align));
        break;
      case A_TK_FG:
        luaA_pushcolor(L, &(*wibox)->sw.ctx.fg);
        break;
      case A_TK_BG:
        luaA_pushcolor(L, &(*wibox)->sw.ctx.bg);
        break;
      case A_TK_POSITION:
        lua_pushstring(L, position_tostr((*wibox)->position));
        break;
      case A_TK_ONTOP:
        lua_pushboolean(L, (*wibox)->ontop);
        break;
      case A_TK_ORIENTATION:
        lua_pushstring(L, orientation_tostr((*wibox)->sw.orientation));
        break;
      case A_TK_WIDGETS:
        if((*wibox)->widgets_table != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, (*wibox)->widgets_table);
        else
            lua_pushnil(L);
        break;
      case A_TK_CURSOR:
        lua_pushstring(L, (*wibox)->cursor);
        break;
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
    wibox_t **wibox = luaA_checkudata(L, 1, "wibox");

    if(lua_gettop(L) == 2)
    {
        area_t wingeom;

        luaA_checktable(L, 2);
        wingeom.x = luaA_getopt_number(L, 2, "x", (*wibox)->sw.geometry.x);
        wingeom.y = luaA_getopt_number(L, 2, "y", (*wibox)->sw.geometry.y);
        wingeom.width = luaA_getopt_number(L, 2, "width", (*wibox)->sw.geometry.width);
        wingeom.height = luaA_getopt_number(L, 2, "height", (*wibox)->sw.geometry.height);

        switch((*wibox)->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            wibox_resize(*wibox, wingeom.width, wingeom.height);
            break;
          case WIBOX_TYPE_NORMAL:
            if((*wibox)->position == Floating)
                wibox_moveresize(*wibox, wingeom);
            else if(wingeom.width != (*wibox)->sw.geometry.width
                    || wingeom.height != (*wibox)->sw.geometry.height)
            {
                wibox_resize(*wibox, wingeom.width, wingeom.height);
                globalconf.screens[(*wibox)->screen].need_arrange = true;
            }
            break;
        }
    }

    return luaA_pusharea(L, (*wibox)->sw.geometry);
}

/** Wibox newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_newindex(lua_State *L)
{
    size_t len;
    wibox_t **wibox = luaA_checkudata(L, 1, "wibox");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    awesome_token_t tok;

    switch((tok = a_tokenize(attr, len)))
    {
        bool b;

      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*wibox)->sw.ctx.fg, buf, len)))
                (*wibox)->need_update = true;
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*wibox)->sw.ctx.bg, buf, len)))
                (*wibox)->need_update = true;
        break;
      case A_TK_ALIGN:
        buf = luaL_checklstring(L, 3, &len);
        (*wibox)->align = draw_align_fromstr(buf, len);
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_NORMAL:
            wibox_position_update(*wibox);
            break;
          case WIBOX_TYPE_TITLEBAR:
            titlebar_update_geometry_floating(client_getbytitlebar(*wibox));
            break;
        }
        break;
      case A_TK_POSITION:
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            return luaA_titlebar_newindex(L, *wibox, tok);
          case WIBOX_TYPE_NORMAL:
            if((buf = luaL_checklstring(L, 3, &len)))
                wibox_setposition(*wibox, position_fromstr(buf, len));
            break;
        }
        break;
      case A_TK_CLIENT:
        /* first detach */
        if(lua_isnil(L, 3))
            titlebar_client_detach(client_getbytitlebar(*wibox));
        else
        {
            client_t **c = luaA_checkudata(L, 3, "client");
            titlebar_client_attach(*c, *wibox);
        }
        break;
      case A_TK_CURSOR:
        if((buf = luaL_checkstring(L, 3)))
        {
            uint16_t cursor_font = xcursor_font_fromstr(buf);
            if(cursor_font)
            {
                xcb_cursor_t cursor = xcursor_new(globalconf.connection, cursor_font);
                p_delete(&(*wibox)->cursor);
                (*wibox)->cursor = a_strdup(buf);
                simplewindow_cursor_set(&(*wibox)->sw, cursor);
            }
        }
        break;
      case A_TK_SCREEN:
        if(lua_isnil(L, 3))
        {
            wibox_detach(*wibox);
            titlebar_client_detach(client_getbytitlebar(*wibox));
        }
        else
        {
            int screen = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(screen);
            if(screen != (*wibox)->screen)
            {
                titlebar_client_detach(client_getbytitlebar(*wibox));
                wibox_attach(*wibox, &globalconf.screens[screen]);
            }
        }
        break;
      case A_TK_ONTOP:
        b = luaA_checkboolean(L, 3);
        if(b != (*wibox)->ontop)
        {
            (*wibox)->ontop = b;
            client_stack();
        }
        break;
      case A_TK_ORIENTATION:
        if((buf = luaL_checklstring(L, 3, &len)))
        {
            simplewindow_orientation_set(&(*wibox)->sw, orientation_fromstr(buf, len));
            (*wibox)->need_update = true;
        }
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*wibox)->sw.border.color, buf, len)))
                if((*wibox)->sw.window)
                    simplewindow_border_color_set(&(*wibox)->sw, &(*wibox)->sw.border.color);
        break;
      case A_TK_VISIBLE:
        b = luaA_checkboolean(L, 3);
        if(b != (*wibox)->isvisible)
            switch((*wibox)->type)
            {
              case WIBOX_TYPE_NORMAL:
                wibox_setvisible(*wibox, b);
                break;
              case WIBOX_TYPE_TITLEBAR:
                titlebar_set_visible(*wibox, b);
                break;
            }
        break;
      case A_TK_WIDGETS:
        if(luaA_isloop(L, 3))
        {
            luaA_warn(L, "table is looping, cannot use this as widget table");
            return 0;
        }
        luaA_register(L, 3, &(*wibox)->widgets_table);
        /* recompute widget node list */
        wibox_widgets_table_build(L, *wibox);
        luaA_table2wtable(L);
        break;
      default:
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            return luaA_titlebar_newindex(L, *wibox, tok);
          case WIBOX_TYPE_NORMAL:
            break;
        }
    }

    return 0;
}

const struct luaL_reg awesome_wibox_methods[] =
{
    { "__call", luaA_wibox_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_wibox_meta[] =
{
    { "geometry", luaA_wibox_geometry },
    { "__index", luaA_wibox_index },
    { "__newindex", luaA_wibox_newindex },
    { "__gc", luaA_wibox_gc },
    { "__eq", luaA_wibox_eq },
    { "__tostring", luaA_wibox_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
