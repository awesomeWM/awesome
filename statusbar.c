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

#include "statusbar.h"
#include "screen.h"
#include "widget.h"
#include "ewmh.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, statusbar_t, statusbar, "statusbar", statusbar_ref)
DO_LUA_GC(statusbar_t, statusbar, "statusbar", statusbar_unref)
DO_LUA_EQ(statusbar_t, statusbar, "statusbar")

/** Draw a statusbar.
 * \param statusbar The statusbar to draw.
 */
static void
statusbar_draw(statusbar_t *statusbar)
{
    statusbar->need_update = false;

    if(!statusbar->position)
        return;

    widget_render(statusbar->widgets, statusbar->ctx, statusbar->sw->gc,
                  statusbar->sw->pixmap,
                  statusbar->screen, statusbar->position,
                  statusbar->sw->geometry.x, statusbar->sw->geometry.y,
                  statusbar);

    simplewindow_refresh_pixmap(statusbar->sw);
    xcb_aux_sync(globalconf.connection);
}

/** Statusbar refresh function.
 */
void
statusbar_refresh(void)
{
    int screen;
    statusbar_t *statusbar;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
            if(statusbar->need_update)
                statusbar_draw(statusbar);
}

/** Update the statusbar position. It deletes every statusbar resources and
 * create them back.
 * \param statusbar The statusbar.
 * \param position The new position.
 */
static void
statusbar_position_update(statusbar_t *statusbar, position_t position)
{
    statusbar_t *sb;
    area_t area;
    xcb_pixmap_t dw;
    xcb_screen_t *s = NULL;
    bool ignore = false;

    globalconf.screens[statusbar->screen].need_arrange = true;

    simplewindow_delete(&statusbar->sw);
    draw_context_delete(&statusbar->ctx);

    if((statusbar->position = position) == Off)
        return;

    area = screen_area_get(statusbar->screen,
                           NULL,
                           &globalconf.screens[statusbar->screen].padding);

    /* Top and Bottom statusbar_t have prio */
    for(sb = globalconf.screens[statusbar->screen].statusbar; sb; sb = sb->next)
    {
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
      case Left:
        if(!statusbar->width_user)
            statusbar->width = area.height;
        statusbar->sw =
            simplewindow_new(globalconf.connection, statusbar->phys_screen, 0, 0,
                             statusbar->height, statusbar->width, 0);
        s = xutil_screen_get(globalconf.connection, statusbar->phys_screen);
        /* we need a new pixmap this way [     ] to render */
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection,
                          s->root_depth, dw, s->root,
                          statusbar->width, statusbar->height);
        statusbar->ctx = draw_context_new(globalconf.connection,
                                          statusbar->phys_screen,
                                          statusbar->width,
                                          statusbar->height,
                                          dw,
                                          &statusbar->colors.fg,
                                          &statusbar->colors.bg);
        break;
      default:
        if(!statusbar->width_user)
            statusbar->width = area.width;
        statusbar->sw =
            simplewindow_new(globalconf.connection, statusbar->phys_screen, 0, 0,
                             statusbar->width, statusbar->height, 0);
        statusbar->ctx = draw_context_new(globalconf.connection,
                                          statusbar->phys_screen,
                                          statusbar->width,
                                          statusbar->height,
                                          statusbar->sw->pixmap,
                                          &statusbar->colors.fg,
                                          &statusbar->colors.bg);
        break;
    }

    switch(statusbar->position)
    {
      default:
        switch(statusbar->align)
        {
          default:
            simplewindow_move(statusbar->sw, area.x, area.y);
            break;
          case AlignRight:
            simplewindow_move(statusbar->sw,
                              area.x + area.width - statusbar->width, area.y);
            break;
          case AlignCenter:
            simplewindow_move(statusbar->sw,
                              area.x + (area.width - statusbar->width) / 2, area.y);
            break;
        }
        break;
      case Bottom:
        switch(statusbar->align)
        {
          default:
            simplewindow_move(statusbar->sw,
                              area.x, (area.y + area.height) - statusbar->height);
            break;
          case AlignRight:
            simplewindow_move(statusbar->sw,
                              area.x + area.width - statusbar->width,
                              (area.y + area.height) - statusbar->height);
            break;
          case AlignCenter:
            simplewindow_move(statusbar->sw,
                              area.x + (area.width - statusbar->width) / 2,
                              (area.y + area.height) - statusbar->height);
            break;
        }
        break;
      case Left:
        switch(statusbar->align)
        {
          default:
            simplewindow_move(statusbar->sw, area.x,
                              (area.y + area.height) - statusbar->sw->geometry.height);
            break;
          case AlignRight:
            simplewindow_move(statusbar->sw, area.x, area.y);
            break;
          case AlignCenter:
            simplewindow_move(statusbar->sw, area.x, (area.y + area.height - statusbar->width) / 2);
        }
        break;
      case Right:
        switch(statusbar->align)
        {
          default:
            simplewindow_move(statusbar->sw, area.x + area.width - statusbar->height, area.y);
            break;
          case AlignRight:
            simplewindow_move(statusbar->sw, area.x + area.width - statusbar->height,
                              area.y + area.height - statusbar->width);
            break;
          case AlignCenter:
            simplewindow_move(statusbar->sw, area.x + area.width - statusbar->height,
                              (area.y + area.height - statusbar->width) / 2);
            break;
        }
        break;
    }

    xcb_map_window(globalconf.connection, statusbar->sw->window);

    /* Set need update */
    statusbar->need_update = true;
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

/** Add a widget to a statusbar.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A statusbar.
 * \lparam A widget.
 */
static int
luaA_statusbar_widget_add(lua_State *L)
{
    statusbar_t **sb = luaA_checkudata(L, 1, "statusbar");
    widget_t **widget = luaA_checkudata(L, 2, "widget");
    widget_node_t *witer, *w = p_new(widget_node_t, 1);

    /* check that there is not already a widget with that name in the titlebar */
    for(witer = (*sb)->widgets; witer; witer = witer->next)
        if(witer->widget != *widget
           && !a_strcmp(witer->widget->name, (*widget)->name))
            luaL_error(L, "a widget with name `%s' already on statusbar `%s'",
                       witer->widget->name, (*sb)->name);

    (*sb)->need_update = true;
    w->widget = *widget;
    widget_node_list_append(&(*sb)->widgets, w);
    widget_ref(widget);

    return 0;
}

/** Remove a widget from a statusbar.
 * \param L The Lua VM State.
 *
 * \luastack
 * \lvalue A statusbar.
 * \lparam A widget.
 */
static int
luaA_statusbar_widget_remove(lua_State *L)
{
    statusbar_t **sb = luaA_checkudata(L, 1, "statusbar");
    widget_t **widget = luaA_checkudata(L, 2, "widget");
    widget_node_t *w, *wnext;

    for(w = (*sb)->widgets; w; w = wnext)
    {
        wnext = w->next;
        if(w->widget == *widget)
        {
            widget_unref(widget);
            widget_node_list_detach(&(*sb)->widgets, w);
            p_delete(&w);
            (*sb)->need_update = true;
        }
    }

    return 0;
}

/** Add the statusbar on a screen.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A stausbar
 * \lparam A screen number.
 */
static int
luaA_statusbar_add(lua_State *L)
{
    statusbar_t *s, **sb = luaA_checkudata(L, 1, "statusbar");
    int i, screen = luaL_checknumber(L, 2) - 1;

    luaA_checkscreen(screen);

    /* Check for uniq name and id. */
    for(i = 0; i < globalconf.screens_info->nscreen; i++)
        for(s = globalconf.screens[i].statusbar; s; s = s->next)
        {
            if(s == *sb)
                luaL_error(L, "this statusbar is already on screen %d",
                           s->screen + 1);
            if(!a_strcmp(s->name, (*sb)->name))
                luaL_error(L, "a statusbar with that name is already on screen %d\n",
                           s->screen + 1);
        }

    (*sb)->screen = screen;
    (*sb)->phys_screen = screen_virttophys(screen);

    statusbar_list_append(&globalconf.screens[screen].statusbar, *sb);
    statusbar_ref(sb);

    /* All the other statusbar and ourselves need to be repositioned */
    for(s = globalconf.screens[screen].statusbar; s; s = s->next)
        statusbar_position_update(s, s->position);

    ewmh_update_workarea((*sb)->phys_screen);

    return 0;
}

/** Remove the statusbar from its screen.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A statusbar
 */
static int
luaA_statusbar_remove(lua_State *L)
{
    statusbar_t *s, **sb = luaA_checkudata(L, 1, "statusbar");
    int i;

    for(i = 0; i < globalconf.screens_info->nscreen; i++)
        for(s = globalconf.screens[i].statusbar; s; s = s->next)
            if(s == *sb)
            {
                statusbar_position_update(*sb, Off);
                statusbar_list_detach(&globalconf.screens[i].statusbar, *sb);
                statusbar_unref(sb);
                globalconf.screens[i].need_arrange = true;
                return 0;
            }

    luaL_error(L, "unable to remove statusbar: not on any screen");

    return 0;
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

    luaA_checktable(L, 2);

    if(!(buf = luaA_getopt_string(L, 2, "name", NULL)))
        luaL_error(L, "object statusbar must have a name");

    sb = p_new(statusbar_t, 1);

    sb->name = a_strdup(buf);

    if(!(buf = luaA_getopt_string(L, 2, "fg", NULL))
       || !xcolor_init(&sb->colors.fg, globalconf.connection, globalconf.default_screen, buf))
    {
        sb->colors.fg = globalconf.colors.fg;
    }

    if(!(buf = luaA_getopt_string(L, 2, "bg", NULL))
       || !xcolor_init(&sb->colors.bg, globalconf.connection,
                       globalconf.default_screen, buf))
    {
        sb->colors.bg = globalconf.colors.bg;
    }

    buf = luaA_getopt_lstring(L, 2, "align", "left", &len);
    sb->align = draw_align_fromstr(buf, len);

    sb->width = luaA_getopt_number(L, 2, "width", 0);
    if(sb->width > 0)
        sb->width_user = true;
    sb->height = luaA_getopt_number(L, 2, "height", 0);
    if(sb->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        sb->height = 1.5 * globalconf.font->height;

    buf = luaA_getopt_lstring(L, 2, "position", "top", &len);
    sb->position = position_fromstr(buf, len);

    return luaA_statusbar_userdata_new(L, sb);
}

/** Get all widget from a statusbar.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A statusbar.
 * \lreturn A table with all widgets from the statusbar.
 */
static int
luaA_statusbar_widget_get(lua_State *L)
{
    statusbar_t **sb = luaA_checkudata(L, 1, "statusbar");
    widget_node_t *witer;

    lua_newtable(L);

    for(witer = (*sb)->widgets; witer; witer = witer->next)
    {
        luaA_widget_userdata_new(L, witer->widget);
        /* ref again for the list */
        widget_ref(&witer->widget);
        lua_setfield(L, -2, witer->widget->name);
    }

    return 1;
}

/** Statusbar index.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
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

/** Statusbar newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_statusbar_newindex(lua_State *L)
{
    size_t len;
    statusbar_t *s, **statusbar = luaA_checkudata(L, 1, "statusbar");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    position_t p;

    switch(a_tokenize(attr, len))
    {
      case A_TK_ALIGN:
        buf = luaL_checklstring(L, 3, &len);
        (*statusbar)->align = draw_align_fromstr(buf, len);
        statusbar_position_update(*statusbar, (*statusbar)->position);
        break;
      case A_TK_FG:
        if((buf = luaL_checkstring(L, 3))
           && xcolor_init(&(*statusbar)->colors.fg, globalconf.connection,
                          globalconf.default_screen, buf))
        {
            if((*statusbar)->ctx)
                (*statusbar)->ctx->fg = (*statusbar)->colors.fg;
            (*statusbar)->need_update = true;
        }
        break;
      case A_TK_BG:
        if((buf = luaL_checkstring(L, 3))
           && xcolor_init(&(*statusbar)->colors.bg, globalconf.connection,
                          globalconf.default_screen, buf))
        {
            if((*statusbar)->ctx)
                (*statusbar)->ctx->bg = (*statusbar)->colors.bg;

            (*statusbar)->need_update = true;
        }
        break;
      case A_TK_POSITION:
        buf = luaL_checklstring(L, 3, &len);
        p = position_fromstr(buf, len);
        if(p != (*statusbar)->position)
        {
            (*statusbar)->position = p;
            for(s = globalconf.screens[(*statusbar)->screen].statusbar; s; s = s->next)
                statusbar_position_update(s, s->position);
            ewmh_update_workarea((*statusbar)->phys_screen);
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
    { "widget_add", luaA_statusbar_widget_add },
    { "widget_remove", luaA_statusbar_widget_remove },
    { "widget_get", luaA_statusbar_widget_get },
    { "add", luaA_statusbar_add },
    { "remove", luaA_statusbar_remove },
    { "__index", luaA_statusbar_index },
    { "__newindex", luaA_statusbar_newindex },
    { "__gc", luaA_statusbar_gc },
    { "__eq", luaA_statusbar_eq },
    { "__tostring", luaA_statusbar_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
