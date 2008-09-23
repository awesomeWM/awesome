/*
 * statusbar.c - statusbar functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb.h>

#include "client.h"
#include "statusbar.h"
#include "screen.h"
#include "widget.h"
#include "wibox.h"
#include "ewmh.h"

extern awesome_t globalconf;

/** Kick out systray windows.
 * \param phys_screen Physical screen number.
 */
static void
statusbar_systray_kickout(int phys_screen)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);

    /* Who! Check that we're not deleting a statusbar with a systray, because it
     * may be its parent. If so, we reparent to root before, otherwise it will
     * hurt very much. */
    xcb_reparent_window(globalconf.connection,
                        globalconf.screens[phys_screen].systray.window,
                        s->root, -512, -512);

    globalconf.screens[phys_screen].systray.parent = s->root;
}

static void
statusbar_systray_refresh(wibox_t *statusbar)
{
    widget_node_t *systray;

    if(statusbar->screen == SCREEN_UNDEF)
        return;

    for(systray = statusbar->widgets; systray; systray = systray->next)
        if(systray->widget->type == systray_new)
        {
            uint32_t config_back[] = { statusbar->colors.bg.pixel };
            uint32_t config_win_vals[4];
            uint32_t config_win_vals_off[2] = { -512, -512 };
            xembed_window_t *em;
            position_t pos;
            int phys_screen = statusbar->sw.ctx.phys_screen;

            if(statusbar->isvisible
               && systray->widget->isvisible
               && systray->area.width)
            {
                pos = statusbar->position;

                /* Set background of the systray window. */
                xcb_change_window_attributes(globalconf.connection,
                                             globalconf.screens[phys_screen].systray.window,
                                             XCB_CW_BACK_PIXEL, config_back);
                /* Map it. */
                xcb_map_window(globalconf.connection, globalconf.screens[phys_screen].systray.window);
                /* Move it. */
                switch(statusbar->position)
                {
                  case Left:
                    config_win_vals[0] = systray->area.y;
                    config_win_vals[1] = statusbar->sw.geometry.height - systray->area.x - systray->area.width;
                    config_win_vals[2] = systray->area.height;
                    config_win_vals[3] = systray->area.width;
                    break;
                  case Right:
                    config_win_vals[0] = systray->area.y;
                    config_win_vals[1] = systray->area.x;
                    config_win_vals[2] = systray->area.height;
                    config_win_vals[3] = systray->area.width;
                    break;
                  default:
                    config_win_vals[0] = systray->area.x;
                    config_win_vals[1] = systray->area.y;
                    config_win_vals[2] = systray->area.width;
                    config_win_vals[3] = systray->area.height;
                    break;
                }
                /* reparent */
                if(globalconf.screens[phys_screen].systray.parent != statusbar->sw.window)
                {
                    xcb_reparent_window(globalconf.connection,
                                        globalconf.screens[phys_screen].systray.window,
                                        statusbar->sw.window,
                                        config_win_vals[0], config_win_vals[1]);
                    globalconf.screens[phys_screen].systray.parent = statusbar->sw.window;
                }
                xcb_configure_window(globalconf.connection,
                                     globalconf.screens[phys_screen].systray.window,
                                     XCB_CONFIG_WINDOW_X
                                     | XCB_CONFIG_WINDOW_Y
                                     | XCB_CONFIG_WINDOW_WIDTH
                                     | XCB_CONFIG_WINDOW_HEIGHT,
                                     config_win_vals);
                /* width = height = systray height */
                config_win_vals[2] = config_win_vals[3] = systray->area.height;
                config_win_vals[0] = 0;
            }
            else
            {
                xcb_unmap_window(globalconf.connection, globalconf.screens[phys_screen].systray.window);
                statusbar_systray_kickout(phys_screen);
                break;
            }

            switch(pos)
            {
              case Left:
                config_win_vals[1] = systray->area.width - config_win_vals[3];
                for(em = globalconf.embedded; em; em = em->next)
                    if(em->phys_screen == phys_screen)
                    {
                        if(config_win_vals[1] - config_win_vals[2] >= (uint32_t) statusbar->sw.geometry.y)
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
                        if(config_win_vals[1] + config_win_vals[3] <= (uint32_t) statusbar->sw.geometry.y + statusbar->sw.geometry.width)
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
              case Top:
              case Bottom:
                config_win_vals[1] = 0;
                for(em = globalconf.embedded; em; em = em->next)
                    if(em->phys_screen == phys_screen)
                    {
                        /* if(x + width < systray.x + systray.width) */
                        if(config_win_vals[0] + config_win_vals[2] <= (uint32_t) AREA_RIGHT(systray->area) + statusbar->sw.geometry.x)
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

/** Draw a statusbar.
 * \param statusbar The statusbar to draw.
 */
static void
statusbar_draw(wibox_t *statusbar)
{
    statusbar->need_update = false;

    if(statusbar->isvisible)
    {
        widget_render(statusbar->widgets, &statusbar->sw.ctx, statusbar->sw.gc,
                      statusbar->sw.pixmap,
                      statusbar->screen, statusbar->position,
                      statusbar->sw.geometry.x, statusbar->sw.geometry.y,
                      statusbar);
        simplewindow_refresh_pixmap(&statusbar->sw);
    }

    statusbar_systray_refresh(statusbar);
}

/** Get a statusbar by its window.
 * \param w The window id.
 * \return A statusbar if found, NULL otherwise.
 */
wibox_t *
statusbar_getbywin(xcb_window_t w)
{
    for(int screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].statusbars.len; i++)
        {
            wibox_t *s = globalconf.screens[screen].statusbars.tab[i];
            if(s->sw.window == w)
                return s;
        }
    return NULL;
}

/** Statusbar refresh function.
 */
void
statusbar_refresh(void)
{
    for(int screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].statusbars.len; i++)
        {
            wibox_t *s = globalconf.screens[screen].statusbars.tab[i];
            if(s->need_update)
                statusbar_draw(s);
        }
}

/** Update the statusbar position. It deletes every statusbar resources and
 * create them back.
 * \param statusbar The statusbar.
 */
void
statusbar_position_update(wibox_t *statusbar)
{
    area_t area;
    bool ignore = false;

    globalconf.screens[statusbar->screen].need_arrange = true;

    if(!statusbar->isvisible)
    {
        xcb_unmap_window(globalconf.connection, statusbar->sw.window);
        /* kick out systray if needed */
        statusbar_systray_refresh(statusbar);
        return;
    }

    area = screen_area_get(statusbar->screen, NULL,
                           &globalconf.screens[statusbar->screen].padding, true);

    /* Top and Bottom wibox_t have prio */
    for(int i = 0; i < globalconf.screens[statusbar->screen].statusbars.len; i++)
    {
        wibox_t *sb = globalconf.screens[statusbar->screen].statusbars.tab[i];
        /* Ignore every statusbar after me that is in the same position */
        if(statusbar == sb)
        {
            ignore = true;
            continue;
        }
        else if(ignore && statusbar->position == sb->position)
            continue;
        switch(sb->position)
        {
          case Left:
            switch(statusbar->position)
            {
              case Left:
                area.x += statusbar->geometry.height;
                break;
              default:
                break;
            }
            break;
          case Right:
            switch(statusbar->position)
            {
              case Right:
                area.x -= statusbar->geometry.height;
                break;
              default:
                break;
            }
            break;
          case Top:
            switch(statusbar->position)
            {
              case Top:
                area.y += sb->geometry.height;
                break;
              case Left:
              case Right:
                area.height -= sb->geometry.height;
                area.y += sb->geometry.height;
                break;
              default:
                break;
            }
            break;
          case Bottom:
            switch(statusbar->position)
            {
              case Bottom:
                area.y -= sb->geometry.height;
                break;
              case Left:
              case Right:
                area.height -= sb->geometry.height;
                break;
              default:
                break;
            }
            break;
        }
    }

    switch(statusbar->position)
    {
      case Right:
        statusbar->geometry.height = area.height;
        statusbar->geometry.width = 1.5 * globalconf.font->height;
        switch(statusbar->align)
        {
          default:
            statusbar->geometry.x = area.x + area.width - statusbar->geometry.width;
            statusbar->geometry.y = area.y;
            break;
          case AlignRight:
            statusbar->geometry.x = area.x + area.width - statusbar->geometry.width;
            statusbar->geometry.y = area.y + area.height - statusbar->geometry.height;
            break;
          case AlignCenter:
            statusbar->geometry.x = area.x + area.width - statusbar->geometry.width;
            statusbar->geometry.y = (area.y + area.height - statusbar->geometry.height) / 2;
            break;
        }
        break;
      case Left:
        statusbar->geometry.height = area.height;
        statusbar->geometry.width = 1.5 * globalconf.font->height;
        switch(statusbar->align)
        {
          default:
            statusbar->geometry.x = area.x;
            statusbar->geometry.y = (area.y + area.height) - statusbar->geometry.height;
            break;
          case AlignRight:
            statusbar->geometry.x = area.x;
            statusbar->geometry.y = area.y;
            break;
          case AlignCenter:
            statusbar->geometry.x = area.x;
            statusbar->geometry.y = (area.y + area.height - statusbar->geometry.height) / 2;
        }
        break;
      case Bottom:
        statusbar->geometry.width = area.width;
        statusbar->geometry.height = 1.5 * globalconf.font->height;
        switch(statusbar->align)
        {
          default:
            statusbar->geometry.x = area.x;
            statusbar->geometry.y = (area.y + area.height) - statusbar->geometry.height;
            break;
          case AlignRight:
            statusbar->geometry.x = area.x + area.width - statusbar->geometry.width;
            statusbar->geometry.y = (area.y + area.height) - statusbar->geometry.height;
            break;
          case AlignCenter:
            statusbar->geometry.x = area.x + (area.width - statusbar->geometry.width) / 2;
            statusbar->geometry.y = (area.y + area.height) - statusbar->geometry.height;
            break;
        }
        break;
      default:
        statusbar->geometry.width = area.width;
        statusbar->geometry.height = 1.5 * globalconf.font->height;
        switch(statusbar->align)
        {
          default:
            statusbar->geometry.x = area.x;
            statusbar->geometry.y = area.y;
            break;
          case AlignRight:
            statusbar->geometry.x = area.x + area.width - statusbar->geometry.width;
            statusbar->geometry.y = area.y;
            break;
          case AlignCenter:
            statusbar->geometry.x = area.x + (area.width - statusbar->geometry.width) / 2;
            statusbar->geometry.y = area.y;
            break;
        }
        break;
    }

    if(!statusbar->sw.window)
    {
        int phys_screen = screen_virttophys(statusbar->screen);

        simplewindow_init(&statusbar->sw, phys_screen,
                          statusbar->geometry.x, statusbar->geometry.y,
                          statusbar->geometry.width, statusbar->geometry.height,
                          0, statusbar->position,
                          &statusbar->colors.fg, &statusbar->colors.bg);
        statusbar->need_update = true;
    }
    /* same window size and position ? */
    else
    {
        if(statusbar->geometry.width != statusbar->sw.geometry.width
           || statusbar->geometry.height != statusbar->sw.geometry.height)
        {
            simplewindow_resize(&statusbar->sw, statusbar->geometry.width, statusbar->geometry.height);
            statusbar->need_update = true;
        }

        if(statusbar->geometry.x != statusbar->sw.geometry.x
            || statusbar->geometry.y != statusbar->sw.geometry.y)
            simplewindow_move(&statusbar->sw, statusbar->geometry.x, statusbar->geometry.y);
    }

    xcb_map_window(globalconf.connection, statusbar->sw.window);
}

/** Create a new statusbar (DEPRECATED).
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with optionaly defined values:
 * position, align, fg, bg, width and height.
 * \lreturn A brand new statusbar.
 */
static int
luaA_statusbar_new(lua_State *L)
{
    deprecate();

    return luaA_wibox_new(L);
}

/** Remove a statubar from a screen.
 * \param statusbar Statusbar to detach from screen.
 */
void
statusbar_detach(wibox_t *statusbar)
{
    if(statusbar->screen != SCREEN_UNDEF)
    {
        bool v;

        /* save visible state */
        v = statusbar->isvisible;
        statusbar->isvisible = false;
        statusbar_position_update(statusbar);
        /* restore position */
        statusbar->isvisible = v;

        simplewindow_wipe(&statusbar->sw);

        for(int i = 0; i < globalconf.screens[statusbar->screen].statusbars.len; i++)
            if(globalconf.screens[statusbar->screen].statusbars.tab[i] == statusbar)
            {
                wibox_array_take(&globalconf.screens[statusbar->screen].statusbars, i);
                break;
            }
        globalconf.screens[statusbar->screen].need_arrange = true;
        statusbar->screen = SCREEN_UNDEF;
        statusbar->type = WIBOX_TYPE_NONE;
        wibox_unref(&statusbar);
    }
}

/** Attach a statusbar.
 * \param statusbar The statusbar to attach.
 * \param s The screen to attach the statusbar to.
 */
void
statusbar_attach(wibox_t *statusbar, screen_t *s)
{
    statusbar_detach(statusbar);

    statusbar->screen = s->index;

    wibox_array_append(&s->statusbars, wibox_ref(&statusbar));

    statusbar->type = WIBOX_TYPE_STATUSBAR;

    /* All the other statusbar and ourselves need to be repositioned */
    for(int i = 0; i < s->statusbars.len; i++)
        statusbar_position_update(s->statusbars.tab[i]);

    ewmh_update_workarea(screen_virttophys(s->index));
}

/** Statusbar newindex.
 * \param L The Lua VM state.
 * \param statusbar The wibox statusbar.
 * \param tok The token for property.
 * \return The number of elements pushed on stack.
 */
int
luaA_statusbar_newindex(lua_State *L, wibox_t *statusbar, awesome_token_t tok)
{
    size_t len;
    const char *buf;
    position_t p;

    switch(tok)
    {
      case A_TK_ALIGN:
        buf = luaL_checklstring(L, 3, &len);
        statusbar->align = draw_align_fromstr(buf, len);
        statusbar_position_update(statusbar);
        break;
      case A_TK_POSITION:
        buf = luaL_checklstring(L, 3, &len);
        p = position_fromstr(buf, len);
        if(p != statusbar->position)
        {
            statusbar->position = p;
            simplewindow_wipe(&statusbar->sw);
            statusbar->sw.window = 0;
            if(statusbar->screen != SCREEN_UNDEF)
            {
                for(int i = 0; i < globalconf.screens[statusbar->screen].statusbars.len; i++)
                {
                    wibox_t *s = globalconf.screens[statusbar->screen].statusbars.tab[i];
                    statusbar_position_update(s);
                }
                ewmh_update_workarea(screen_virttophys(statusbar->screen));
            }
        }
        break;
      default:
        return 0;
    }

    return 0;
}

const struct luaL_reg awesome_statusbar_methods[] =
{
    { "__call", luaA_statusbar_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_statusbar_meta[] =
{
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
