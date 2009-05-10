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
    luaA_ref_array_wipe(&wibox->refs);
    p_delete(&wibox->cursor);
    simplewindow_wipe(&wibox->sw);
    button_array_wipe(&wibox->buttons);
    image_unref(L, wibox->bg_image);
    luaL_unref(L, LUA_REGISTRYINDEX, wibox->widgets_table);
    luaL_unref(L, LUA_REGISTRYINDEX, wibox->mouse_enter);
    luaL_unref(L, LUA_REGISTRYINDEX, wibox->mouse_leave);
    widget_node_array_wipe(&wibox->widgets);
    return 0;
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

static void
wibox_map(wibox_t *wibox)
{
    xcb_map_window(globalconf.connection, wibox->sw.window);
    /* We must make sure the wibox does not display garbage */
    wibox_need_update(wibox);
    /* Stack this wibox correctly */
    client_stack();
}

static void
wibox_move(wibox_t *wibox, int16_t x, int16_t y)
{
    if(wibox->sw.window)
    {
        simplewindow_move(&wibox->sw, x, y);
        wibox->screen = screen_getbycoord(wibox->screen, x, y);
    }
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
    wibox_need_update(wibox);
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
            uint32_t config_back[] = { wibox->sw.ctx.bg.pixel };
            uint32_t config_win_vals[4];
            uint32_t config_win_vals_off[2] = { -512, -512 };
            xembed_window_t *em;
            int phys_screen = wibox->sw.ctx.phys_screen;

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
                switch(wibox->sw.orientation)
                {
                  case North:
                    config_win_vals[0] = systray->geometry.y;
                    config_win_vals[1] = wibox->sw.geometry.height - systray->geometry.x - systray->geometry.width;
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
                if(globalconf.screens.tab[phys_screen].systray.parent != wibox->sw.window)
                {
                    xcb_reparent_window(globalconf.connection,
                                        globalconf.screens.tab[phys_screen].systray.window,
                                        wibox->sw.window,
                                        config_win_vals[0], config_win_vals[1]);
                    globalconf.screens.tab[phys_screen].systray.parent = wibox->sw.window;
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

            switch(wibox->sw.orientation)
            {
              case North:
                config_win_vals[1] = systray->geometry.width - config_win_vals[3];
                for(int j = 0; j < globalconf.embedded.len; j++)
                {
                    em = &globalconf.embedded.tab[j];
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
                        if(config_win_vals[1] + config_win_vals[3] <= (uint32_t) wibox->sw.geometry.y + AREA_RIGHT(systray->geometry))
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
                }
                break;
            }
            break;
        }
    }
}

/** Get a wibox by its window.
 * \param w The window id.
 * \return A wibox if found, NULL otherwise.
 */
wibox_t *
wibox_getbywin(xcb_window_t win)
{
    foreach(w, globalconf.wiboxes)
        if((*w)->sw.window == win)
            return *w;

    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        if(c->titlebar && c->titlebar->sw.window == win)
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
    foreach(w, globalconf.wiboxes)
        if((*w)->need_update)
            wibox_draw(*w);

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
                xcb_unmap_window(globalconf.connection, wibox->sw.window);

            /* kick out systray if needed */
            wibox_systray_refresh(wibox);
        }

        hook_property(wibox, wibox, "visible");
    }
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

        simplewindow_wipe(&wibox->sw);

        wibox->screen->need_arrange = true;

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

    wibox_t *wibox = wibox_ref(globalconf.L);

    wibox_detach(wibox);

    /* Set the wibox screen */
    wibox->screen = s;

    /* Check that the wibox coordinates matches the screen. */
    screen_t *cscreen =
        screen_getbycoord(wibox->screen, wibox->sw.geometry.x, wibox->sw.geometry.y);

    /* If it does not match, move it to the screen coordinates */
    if(cscreen != wibox->screen)
        wibox_move(wibox, s->geometry.x, s->geometry.y);

    wibox_array_append(&globalconf.wiboxes, wibox);

    simplewindow_init(&wibox->sw, phys_screen,
                      wibox->sw.geometry,
                      wibox->sw.border.width,
                      wibox->sw.orientation,
                      &wibox->sw.ctx.fg, &wibox->sw.ctx.bg);

    simplewindow_border_color_set(&wibox->sw, &wibox->sw.border.color);

    simplewindow_cursor_set(&wibox->sw,
                            xcursor_new(globalconf.connection, xcursor_font_fromstr(wibox->cursor)));
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

    w->ontop = luaA_getopt_boolean(L, 2, "ontop", false);

    w->sw.border.width = luaA_getopt_number(L, 2, "border_width", 0);
    w->sw.geometry.x = luaA_getopt_number(L, 2, "x", 0);
    w->sw.geometry.y = luaA_getopt_number(L, 2, "y", 0);
    w->sw.geometry.width = luaA_getopt_number(L, 2, "width", 100);
    w->sw.geometry.height = luaA_getopt_number(L, 2, "height", globalconf.font->height * 1.5);

    w->isvisible = true;
    w->cursor = a_strdup("left_ptr");

    w->mouse_enter = w->mouse_leave = LUA_REFNIL;

    for(i = 0; i <= reqs_nbr; i++)
        xcolor_init_reply(reqs[i]);

    return 1;
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
    wibox_need_update(wibox);
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
    foreach(w, globalconf.wiboxes)
    {
        wibox_t *wibox = *w;
        if(luaA_wibox_hasitem(L, wibox, item))
        {
            /* recompute widget node list */
            wibox_widgets_table_build(L, wibox);
            lua_pop(L, 1); /* remove widgets table */
        }

    }

    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        if(c->titlebar && luaA_wibox_hasitem(L, c->titlebar, item))
        {
            /* recompute widget node list */
            wibox_widgets_table_build(L, c->titlebar);
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
        lua_pushnumber(L, wibox->sw.border.width);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushxcolor(L, &wibox->sw.border.color);
        break;
      case A_TK_ALIGN:
        if(wibox->type == WIBOX_TYPE_NORMAL)
            luaA_deprecate(L, "awful.wibox.align");
        lua_pushstring(L, draw_align_tostr(wibox->align));
        break;
      case A_TK_FG:
        luaA_pushxcolor(L, &wibox->sw.ctx.fg);
        break;
      case A_TK_BG:
        luaA_pushxcolor(L, &wibox->sw.ctx.bg);
        break;
      case A_TK_BG_IMAGE:
        image_push(L, wibox->bg_image);
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
        lua_pushstring(L, orientation_tostr(wibox->sw.orientation));
        break;
      case A_TK_WIDGETS:
        if(wibox->widgets_table != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, wibox->widgets_table);
        else
            lua_pushnil(L);
        break;
      case A_TK_CURSOR:
        lua_pushstring(L, wibox->cursor);
        break;
      case A_TK_OPACITY:
        {
            double d;
            if ((d = window_opacity_get(wibox->sw.window)) >= 0)
                lua_pushnumber(L, d);
            else
                return 0;
        }
        break;
      case A_TK_MOUSE_ENTER:
        if(wibox->mouse_enter != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, wibox->mouse_enter);
        else
            return 0;
        return 1;
      case A_TK_MOUSE_LEAVE:
        if(wibox->mouse_leave != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, wibox->mouse_leave);
        else
            return 0;
        return 1;
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
        wingeom.x = luaA_getopt_number(L, 2, "x", wibox->sw.geometry.x);
        wingeom.y = luaA_getopt_number(L, 2, "y", wibox->sw.geometry.y);
        wingeom.width = luaA_getopt_number(L, 2, "width", wibox->sw.geometry.width);
        wingeom.height = luaA_getopt_number(L, 2, "height", wibox->sw.geometry.height);

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

    return luaA_pusharea(L, wibox->sw.geometry);
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
            if(xcolor_init_reply(xcolor_init_unchecked(&wibox->sw.ctx.fg, buf, len)))
                wibox->need_update = true;
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&wibox->sw.ctx.bg, buf, len)))
                wibox->need_update = true;
        break;
      case A_TK_BG_IMAGE:
        image_unref(L, wibox->bg_image);
        wibox->bg_image = image_ref(L);
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
                simplewindow_cursor_set(&wibox->sw, cursor);
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
            simplewindow_orientation_set(&wibox->sw, orientation_fromstr(buf, len));
            wibox_need_update(wibox);
        }
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&wibox->sw.border.color, buf, len)))
                if(wibox->sw.window)
                    simplewindow_border_color_set(&wibox->sw, &wibox->sw.border.color);
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
        /* register object */
        luaA_register(L, 3, &wibox->widgets_table);
        /* duplicate table because next function will eat it */
        lua_pushvalue(L, -1);
        /* recompute widget node list */
        wibox_widgets_table_build(L, wibox);
        luaA_table2wtable(L);
        break;
      case A_TK_OPACITY:
        if(lua_isnil(L, 3))
            window_opacity_set(wibox->sw.window, -1);
        else
        {
            double d = luaL_checknumber(L, 3);
            if(d >= 0 && d <= 1)
                window_opacity_set(wibox->sw.window, d);
        }
        break;
      case A_TK_MOUSE_ENTER:
        luaA_registerfct(L, 3, &wibox->mouse_enter);
        return 0;
      case A_TK_MOUSE_LEAVE:
        luaA_registerfct(L, 3, &wibox->mouse_leave);
        return 0;
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

/** Get or set mouse buttons bindings to a wibox.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A wibox.
 * \lparam An array of mouse button bindings objects, or nothing.
 * \return The array of mouse button bindings objects of this wibox.
 */
static int
luaA_wibox_buttons(lua_State *L)
{
    wibox_t *wibox = luaL_checkudata(L, 1, "wibox");
    button_array_t *buttons = &wibox->buttons;

    if(lua_gettop(L) == 2)
    {
        luaA_button_array_set(L, 2, buttons);
        return 1;
    }

    return luaA_button_array_get(L, buttons);
}

const struct luaL_reg awesome_wibox_methods[] =
{
    { "__call", luaA_wibox_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_wibox_meta[] =
{
    { "buttons", luaA_wibox_buttons },
    { "geometry", luaA_wibox_geometry },
    { "__index", luaA_wibox_index },
    { "__newindex", luaA_wibox_newindex },
    { "__gc", luaA_wibox_gc },
    { "__tostring", luaA_wibox_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
