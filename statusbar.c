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
#include "ewmh.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, statusbar_t, statusbar, "statusbar", statusbar_ref)
DO_LUA_GC(statusbar_t, statusbar, "statusbar", statusbar_unref)
DO_LUA_EQ(statusbar_t, statusbar, "statusbar")

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
statusbar_systray_refresh(statusbar_t *statusbar)
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

            if(statusbar->position
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
                /* hide */
                pos = Off;
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
              default:
                statusbar_systray_kickout(phys_screen);
                break;
            }
            break;
        }
}

/** Draw a statusbar.
 * \param statusbar The statusbar to draw.
 */
static void
statusbar_draw(statusbar_t *statusbar)
{
    statusbar->need_update = false;

    if(statusbar->position)
    {
        widget_render(statusbar->widgets, &statusbar->sw.ctx, statusbar->sw.gc,
                      statusbar->sw.pixmap,
                      statusbar->screen, statusbar->position,
                      statusbar->sw.geometry.x, statusbar->sw.geometry.y,
                      statusbar, AWESOME_TYPE_STATUSBAR);
        simplewindow_refresh_pixmap(&statusbar->sw);
    }

    statusbar_systray_refresh(statusbar);
}

/** Get a statusbar by its window.
 * \param w The window id.
 * \return A statusbar if found, NULL otherwise.
 */
statusbar_t *
statusbar_getbywin(xcb_window_t w)
{
    for(int screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].statusbars.len; i++)
        {
            statusbar_t *s = globalconf.screens[screen].statusbars.tab[i];
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
            statusbar_t *s = globalconf.screens[screen].statusbars.tab[i];
            if(s->need_update)
                statusbar_draw(s);
        }
}

/** Update the statusbar position. It deletes every statusbar resources and
 * create them back.
 * \param statusbar The statusbar.
 */
void
statusbar_position_update(statusbar_t *statusbar)
{
    area_t area, wingeometry;
    bool ignore = false;

    if(statusbar->position == Off)
    {
        xcb_unmap_window(globalconf.connection, statusbar->sw.window);
        /* kick out systray if needed */
        statusbar_systray_refresh(statusbar);
        return;
    }

    area = screen_area_get(statusbar->screen, NULL,
                           &globalconf.screens[statusbar->screen].padding, true);

    /* Top and Bottom statusbar_t have prio */
    for(int i = 0; i < globalconf.screens[statusbar->screen].statusbars.len; i++)
    {
        statusbar_t *sb = globalconf.screens[statusbar->screen].statusbars.tab[i];
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
                area.x += statusbar->height;
                break;
              default:
                break;
            }
            break;
          case Right:
            switch(statusbar->position)
            {
              case Right:
                area.x -= statusbar->height;
                break;
              default:
                break;
            }
            break;
          case Top:
            switch(statusbar->position)
            {
              case Top:
                area.y += sb->height;
                break;
              case Left:
              case Right:
                area.height -= sb->height;
                area.y += sb->height;
                break;
              default:
                break;
            }
            break;
          case Bottom:
            switch(statusbar->position)
            {
              case Bottom:
                area.y -= sb->height;
                break;
              case Left:
              case Right:
                area.height -= sb->height;
                break;
              default:
                break;
            }
            break;
          default:
            break;
        }
    }

    switch(statusbar->position)
    {
      case Right:
        if(statusbar->width > 0)
            wingeometry.height = statusbar->width;
        else
            wingeometry.height = area.height;
        wingeometry.width = statusbar->height;
        switch(statusbar->align)
        {
          default:
            wingeometry.x = area.x + area.width - wingeometry.width;
            wingeometry.y = area.y;
            break;
          case AlignRight:
            wingeometry.x = area.x + area.width - wingeometry.width;
            wingeometry.y = area.y + area.height - wingeometry.height;
            break;
          case AlignCenter:
            wingeometry.x = area.x + area.width - wingeometry.width;
            wingeometry.y = (area.y + area.height - wingeometry.height) / 2;
            break;
        }
        break;
      case Left:
        if(statusbar->width > 0)
            wingeometry.height = statusbar->width;
        else
            wingeometry.height = area.height;
        wingeometry.width = statusbar->height;
        switch(statusbar->align)
        {
          default:
            wingeometry.x = area.x;
            wingeometry.y = (area.y + area.height) - wingeometry.height;
            break;
          case AlignRight:
            wingeometry.x = area.x;
            wingeometry.y = area.y;
            break;
          case AlignCenter:
            wingeometry.x = area.x;
            wingeometry.y = (area.y + area.height - wingeometry.height) / 2;
        }
        break;
      case Bottom:
        if(statusbar->width > 0)
            wingeometry.width = statusbar->width;
        else
            wingeometry.width = area.width;
        wingeometry.height = statusbar->height;
        switch(statusbar->align)
        {
          default:
            wingeometry.x = area.x;
            wingeometry.y = (area.y + area.height) - wingeometry.height;
            break;
          case AlignRight:
            wingeometry.x = area.x + area.width - wingeometry.width;
            wingeometry.y = (area.y + area.height) - wingeometry.height;
            break;
          case AlignCenter:
            wingeometry.x = area.x + (area.width - wingeometry.width) / 2;
            wingeometry.y = (area.y + area.height) - wingeometry.height;
            break;
        }
        break;
      default:
        if(statusbar->width > 0)
            wingeometry.width = statusbar->width;
        else
            wingeometry.width = area.width;
        wingeometry.height = statusbar->height;
        switch(statusbar->align)
        {
          default:
            wingeometry.x = area.x;
            wingeometry.y = area.y;
            break;
          case AlignRight:
            wingeometry.x = area.x + area.width - wingeometry.width;
            wingeometry.y = area.y;
            break;
          case AlignCenter:
            wingeometry.x = area.x + (area.width - wingeometry.width) / 2;
            wingeometry.y = area.y;
            break;
        }
        break;
    }

    if(!statusbar->sw.window)
    {
        int phys_screen = screen_virttophys(statusbar->screen);

        simplewindow_init(&statusbar->sw, phys_screen,
                          wingeometry.x, wingeometry.y,
                          wingeometry.width, wingeometry.height,
                          0, statusbar->position,
                          &statusbar->colors.fg, &statusbar->colors.bg);
        xcb_map_window(globalconf.connection, statusbar->sw.window);
    }
    /* same window size and position ? */
    else
    {
        if(wingeometry.width != statusbar->sw.geometry.width
           || wingeometry.height != statusbar->sw.geometry.height)
        {
            simplewindow_resize(&statusbar->sw, wingeometry.width, wingeometry.height);
            statusbar->need_update = true;
        }

        if(wingeometry.x != statusbar->sw.geometry.x
            || wingeometry.y != statusbar->sw.geometry.y)
            simplewindow_move(&statusbar->sw, wingeometry.x, wingeometry.y);
    }

    /* Set need update */
    globalconf.screens[statusbar->screen].need_arrange = true;
}

/** Convert a statusbar to a printable string.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A statusbar.
 */
static int
luaA_statusbar_tostring(lua_State *L)
{
    statusbar_t **p = luaA_checkudata(L, 1, "statusbar");
    lua_pushfstring(L, "[statusbar udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

/** Create a new statusbar.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with at least a name attribute. Optionaly defined values are:
 * position, align, fg, bg, width and height.
 * \lreturn A brand new statusbar.
 */
static int
luaA_statusbar_new(lua_State *L)
{
    statusbar_t *sb;
    const char *buf;
    size_t len;
    xcolor_init_request_t reqs[2];
    int8_t i, reqs_nbr = -1;

    luaA_checktable(L, 2);

    if(!(buf = luaA_getopt_string(L, 2, "name", NULL)))
        luaL_error(L, "object statusbar must have a name");

    sb = p_new(statusbar_t, 1);

    sb->name = a_strdup(buf);

    sb->colors.fg = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "fg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&sb->colors.fg, buf, len);

    sb->colors.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&sb->colors.bg, buf, len);

    buf = luaA_getopt_lstring(L, 2, "align", "left", &len);
    sb->align = draw_align_fromstr(buf, len);

    sb->width = luaA_getopt_number(L, 2, "width", 0);
    sb->height = luaA_getopt_number(L, 2, "height", 0);
    if(sb->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        sb->height = 1.5 * globalconf.font->height;

    buf = luaA_getopt_lstring(L, 2, "position", "top", &len);
    sb->position = position_fromstr(buf, len);

    sb->screen = SCREEN_UNDEF;

    for(i = 0; i <= reqs_nbr; i++)
        xcolor_init_reply(reqs[i]);

    return luaA_statusbar_userdata_new(L, sb);
}

/** Statusbar object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield screen Screen number.
 * \lfield align The alignment.
 * \lfield fg Foreground color.
 * \lfield bg Background color.
 * \lfield position The position.
 */
static int
luaA_statusbar_index(lua_State *L)
{
    size_t len;
    statusbar_t **statusbar = luaA_checkudata(L, 1, "statusbar");
    const char *attr = luaL_checklstring(L, 2, &len);

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(attr, len))
    {
      case A_TK_SCREEN:
        if((*statusbar)->screen == SCREEN_UNDEF)
            return 0;
        lua_pushnumber(L, (*statusbar)->screen + 1);
        break;
      case A_TK_ALIGN:
        lua_pushstring(L, draw_align_tostr((*statusbar)->align));
        break;
      case A_TK_FG:
        luaA_pushcolor(L, &(*statusbar)->colors.fg);
        break;
      case A_TK_BG:
        luaA_pushcolor(L, &(*statusbar)->colors.bg);
        break;
      case A_TK_POSITION:
        lua_pushstring(L, position_tostr((*statusbar)->position));
        break;
      default:
        return 0;
    }

    return 1;
}

/** Get or set the statusbar widgets.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam None, or a table of widgets to set.
 * \lreturn The current statusbar widgets.
*/
static int
luaA_statusbar_widgets(lua_State *L)
{
    statusbar_t **statusbar = luaA_checkudata(L, 1, "statusbar");

    if(lua_gettop(L) == 2)
    {
        luaA_widget_set(L, 2, *statusbar, &(*statusbar)->widgets);
        (*statusbar)->need_update = true;
        (*statusbar)->mouse_over = NULL;
        return 1;
    }
    return luaA_widget_get(L, (*statusbar)->widgets);
}

/** Remove a statubar from a screen.
 * \param statusbar Statusbar to detach from screen.
 */
static void
statusbar_remove(statusbar_t *statusbar)
{
    if(statusbar->screen != SCREEN_UNDEF)
    {
        position_t p;

        /* save position */
        p = statusbar->position;
        statusbar->position = Off;
        statusbar_position_update(statusbar);
        /* restore position */
        statusbar->position = p;

        simplewindow_wipe(&statusbar->sw);
        /* sw.window is used to now if the window has been init or not */
        statusbar->sw.window = 0;

        for(int i = 0; i < globalconf.screens[statusbar->screen].statusbars.len; i++)
            if(globalconf.screens[statusbar->screen].statusbars.tab[i] == statusbar)
            {
                statusbar_array_take(&globalconf.screens[statusbar->screen].statusbars, i);
                break;
            }
        globalconf.screens[statusbar->screen].need_arrange = true;
        statusbar->screen = SCREEN_UNDEF;
        statusbar_unref(&statusbar);
    }
}

/** Statusbar newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_statusbar_newindex(lua_State *L)
{
    size_t len;
    statusbar_t **statusbar = luaA_checkudata(L, 1, "statusbar");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    position_t p;
    int screen;

    switch(a_tokenize(attr, len))
    {
      case A_TK_SCREEN:
        if(lua_isnil(L, 3))
            statusbar_remove(*statusbar);
        else
        {
            screen = luaL_checknumber(L, 3) - 1;

            luaA_checkscreen(screen);

            if((*statusbar)->screen == screen)
                luaL_error(L, "this statusbar is already on screen %d",
                           (*statusbar)->screen + 1);

            /* Check for uniq name and id. */
            for(int i = 0; i < globalconf.screens[screen].statusbars.len; i++)
            {
                statusbar_t *s = globalconf.screens[screen].statusbars.tab[i];
                if(!a_strcmp(s->name, (*statusbar)->name))
                    luaL_error(L, "a statusbar with that name is already on screen %d\n",
                               screen + 1);
            }

            statusbar_remove(*statusbar);

            (*statusbar)->screen = screen;

            statusbar_array_append(&globalconf.screens[screen].statusbars, *statusbar);
            statusbar_ref(statusbar);

            /* All the other statusbar and ourselves need to be repositioned */
            for(int i = 0; i < globalconf.screens[screen].statusbars.len; i++)
            {
                statusbar_t *s = globalconf.screens[screen].statusbars.tab[i];
                statusbar_position_update(s);
            }

            ewmh_update_workarea(screen_virttophys(screen));
        }
        break;
      case A_TK_ALIGN:
        buf = luaL_checklstring(L, 3, &len);
        (*statusbar)->align = draw_align_fromstr(buf, len);
        statusbar_position_update(*statusbar);
        break;
      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*statusbar)->colors.fg, buf, len)))
            {
                (*statusbar)->sw.ctx.fg = (*statusbar)->colors.fg;
                (*statusbar)->need_update = true;
            }
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*statusbar)->colors.bg, buf, len)))
            {
                (*statusbar)->sw.ctx.bg = (*statusbar)->colors.bg;
                (*statusbar)->need_update = true;
            }
        break;
      case A_TK_POSITION:
        buf = luaL_checklstring(L, 3, &len);
        p = position_fromstr(buf, len);
        if(p != (*statusbar)->position)
        {
            (*statusbar)->position = p;
            simplewindow_wipe(&(*statusbar)->sw);
            (*statusbar)->sw.window = 0;
            if((*statusbar)->screen != SCREEN_UNDEF)
            {
                for(int i = 0; i < globalconf.screens[(*statusbar)->screen].statusbars.len; i++)
                {
                    statusbar_t *s = globalconf.screens[(*statusbar)->screen].statusbars.tab[i];
                    statusbar_position_update(s);
                }
                ewmh_update_workarea(screen_virttophys((*statusbar)->screen));
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
    { "widgets", luaA_statusbar_widgets },
    { "__index", luaA_statusbar_index },
    { "__newindex", luaA_statusbar_newindex },
    { "__gc", luaA_statusbar_gc },
    { "__eq", luaA_statusbar_eq },
    { "__tostring", luaA_statusbar_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
