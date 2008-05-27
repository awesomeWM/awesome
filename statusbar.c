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

#include <stdio.h>
#include <math.h>

#include <pthread.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "statusbar.h"
#include "screen.h"
#include "tag.h"
#include "widget.h"
#include "window.h"

extern awesome_t globalconf;
extern bool running;

static void
statusbar_draw(statusbar_t *statusbar)
{
    widget_node_t *w;
    int left = 0, right = 0;
    area_t rectangle = { 0, 0, 0, 0, NULL, NULL };

    statusbar->need_update.value = false;

    if(!statusbar->position)
        return;

    rectangle.width = statusbar->width;
    rectangle.height = statusbar->height;
    draw_rectangle(statusbar->ctx, rectangle, 1.0, true,
                   statusbar->colors.bg);

    for(w = statusbar->widgets; w; w = w->next)
        if(w->widget->isvisible && w->widget->align == AlignLeft)
            left += w->widget->draw(w, statusbar, left, (left + right));

    /* renders right widget from last to first */
    for(w = *widget_node_list_last(&statusbar->widgets); w; w = w->prev)
        if(w->widget->isvisible && w->widget->align == AlignRight)
            right += w->widget->draw(w, statusbar, right, (left + right));

    for(w = statusbar->widgets; w; w = w->next)
        if(w->widget->isvisible && w->widget->align == AlignFlex)
            left += w->widget->draw(w, statusbar, left, (left + right));

    switch(statusbar->position)
    {
        case Right:
          draw_rotate(statusbar->ctx, statusbar->sw->drawable,
                      statusbar->ctx->height, statusbar->ctx->width,
                      M_PI_2, statusbar->height, 0);
          break;
        case Left:
          draw_rotate(statusbar->ctx, statusbar->sw->drawable,
                      statusbar->ctx->height, statusbar->ctx->width,
                      - M_PI_2, 0, statusbar->width);
          break;
        default:
          break;
    }

    simplewindow_refresh_drawable(statusbar->sw);
}

static void *
statusbar_refresh(void *p)
{
    statusbar_t *statusbar = (statusbar_t *) p;

    while(running)
    {
        pthread_mutex_lock(&statusbar->need_update.lock);

        while(!statusbar->need_update.value)
            pthread_cond_wait(&statusbar->need_update.cond, &statusbar->need_update.lock);

        statusbar_draw(statusbar);
        pthread_mutex_unlock(&statusbar->need_update.lock);
    }

    return NULL;
}

static void
statusbar_position_update(statusbar_t *statusbar, position_t position)
{
    statusbar_t *sb;
    area_t area;
    xcb_drawable_t dw;
    xcb_screen_t *s = NULL;
    bool ignore = false;

    simplewindow_delete(&statusbar->sw);
    draw_context_delete(&statusbar->ctx);

    if((statusbar->position = position) == Off)
        return;

    area = screen_get_area(statusbar->screen,
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
        if(statusbar->width <= 0)
            statusbar->width = area.height;
        statusbar->sw =
            simplewindow_new(globalconf.connection, statusbar->phys_screen, 0, 0,
                             statusbar->height, statusbar->width, 0);
        s = xcb_aux_get_screen(globalconf.connection, statusbar->phys_screen);
        /* we need a new pixmap this way [     ] to render */
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection,
                          s->root_depth, dw, s->root,
                          statusbar->width, statusbar->height);
        statusbar->ctx = draw_context_new(globalconf.connection,
                                          statusbar->phys_screen,
                                          statusbar->width,
                                          statusbar->height,
                                          dw);
        break;
      default:
        if(statusbar->width <= 0)
            statusbar->width = area.width;
        statusbar->sw =
            simplewindow_new(globalconf.connection, statusbar->phys_screen, 0, 0,
                             statusbar->width, statusbar->height, 0);
        statusbar->ctx = draw_context_new(globalconf.connection,
                                          statusbar->phys_screen,
                                          statusbar->width,
                                          statusbar->height,
                                          statusbar->sw->drawable);
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

    globalconf.screens[statusbar->screen].need_arrange = true;
    xcb_map_window(globalconf.connection, statusbar->sw->window);
    statusbar_draw(statusbar);
}

void
statusbar_needupdate(statusbar_t *statusbar)
{
    pthread_mutex_lock(&statusbar->need_update.lock);
    statusbar->need_update.value = true;
    pthread_mutex_unlock(&statusbar->need_update.lock);
    pthread_cond_broadcast(&statusbar->need_update.cond);
}

statusbar_t *
statusbar_getbyname(int screen, const char *name)
{
    statusbar_t *sb;

    for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
        if(!a_strcmp(sb->name, name))
            return sb;

    return NULL;
}

static int
luaA_statusbar_eq(lua_State *L)
{
    statusbar_t **t1 = luaL_checkudata(L, 1, "statusbar");
    statusbar_t **t2 = luaL_checkudata(L, 2, "statusbar");
    lua_pushboolean(L, (*t1 == *t2));
    return 1;
}

static int
luaA_statusbar_position_set(lua_State *L)
{
    statusbar_t *s, **sb = luaL_checkudata(L, 1, "statusbar");
    const char *pos = luaL_checkstring(L, 2);
    position_t position = position_get_from_str(pos);

    if(position != (*sb)->position)
    {
        (*sb)->position = position;
        for(s = globalconf.screens[(*sb)->screen].statusbar; s; s = s->next)
            statusbar_position_update(s, s->position);
    }

    return 0;
}

static int
luaA_statusbar_align_set(lua_State *L)
{
    statusbar_t **sb = luaL_checkudata(L, 1, "statusbar");
    const char *al = luaL_checkstring(L, 2);
    alignment_t align = draw_align_get_from_str(al);

    (*sb)->align = align;
    statusbar_position_update(*sb, (*sb)->position);

    return 0;
}

static int
luaA_statusbar_tostring(lua_State *L)
{
    statusbar_t **p = luaL_checkudata(L, 1, "statusbar");
    lua_pushfstring(L, "[statusbar udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

static int
luaA_statusbar_widget_add(lua_State *L)
{
    statusbar_t **sb = luaL_checkudata(L, 1, "statusbar");
    widget_t **widget = luaL_checkudata(L, 2, "widget");
    widget_node_t *w = p_new(widget_node_t, 1);

    statusbar_needupdate(*sb);
    w->widget = *widget;
    widget_node_list_append(&(*sb)->widgets, w);
    widget_ref(widget);

    return 0;
}

static int
luaA_statusbar_add(lua_State *L)
{
    statusbar_t *s, **sb = luaL_checkudata(L, 1, "statusbar");
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

    statusbar_list_append(&globalconf.screens[screen].statusbar, *sb);
    for(s = globalconf.screens[(*sb)->screen].statusbar; s; s = s->next)
        statusbar_position_update(s, s->position);
    statusbar_ref(sb);
    (*sb)->screen = screen;
    (*sb)->phys_screen = screen_virttophys(screen);

    /* Initialize thread stuffs and start it */
    pthread_cond_init(&(*sb)->need_update.cond, NULL);
    pthread_mutex_init(&(*sb)->need_update.lock, NULL);
    if(pthread_create(&(*sb)->tid, NULL, statusbar_refresh, *sb))
        perror("unable to create thread");

    return 0;
}

static int
luaA_statusbar_remove(lua_State *L)
{
    statusbar_t *s, **sb = luaL_checkudata(L, 1, "statusbar");
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

static int
luaA_statusbar_new(lua_State *L)
{
    statusbar_t **sb = lua_newuserdata(L, sizeof(statusbar_t *));
    int objpos = lua_gettop(L);
    const char *color;

    luaA_checktable(L, 1);

    *sb = p_new(statusbar_t, 1);

    (*sb)->name = luaA_name_init(L);

    lua_getfield(L, 1, "fg");
    if((color = luaL_optstring(L, -1, NULL)))
        draw_color_new(globalconf.connection, globalconf.default_screen,
                       color, &(*sb)->colors.fg);
    else
        (*sb)->colors.fg = globalconf.colors.fg;

    lua_getfield(L, 1, "bg");
    if((color = luaL_optstring(L, -1, NULL)))
        draw_color_new(globalconf.connection, globalconf.default_screen,
                       color, &(*sb)->colors.bg);
    else
        (*sb)->colors.bg = globalconf.colors.bg;

    (*sb)->align = draw_align_get_from_str(luaA_getopt_string(L, 1, "align", "left"));

    (*sb)->width = luaA_getopt_number(L, 1, "width", 0);
    (*sb)->height = luaA_getopt_number(L, 1, "height", 0);
    if((*sb)->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        (*sb)->height = 1.5 * globalconf.font->height;

    (*sb)->position = position_get_from_str(luaA_getopt_string(L, 1, "position", "top"));

    statusbar_ref(sb);

    lua_pushvalue(L, objpos);
    return luaA_settype(L, "statusbar");
}

static int
luaA_statusbar_gc(lua_State *L)
{
    statusbar_t **sb = luaL_checkudata(L, 1, "statusbar");
    statusbar_unref(sb);
    return 0;
}

const struct luaL_reg awesome_statusbar_methods[] =
{
    { "new", luaA_statusbar_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_statusbar_meta[] =
{
    { "widget_add", luaA_statusbar_widget_add },
    { "position_set", luaA_statusbar_position_set },
    { "align_set", luaA_statusbar_align_set },
    { "add", luaA_statusbar_add },
    { "remove", luaA_statusbar_remove },
    { "__gc", luaA_statusbar_gc },
    { "__eq", luaA_statusbar_eq },
    { "__tostring", luaA_statusbar_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
