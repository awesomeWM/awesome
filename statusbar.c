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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "statusbar.h"
#include "screen.h"
#include "tag.h"
#include "widget.h"
#include "window.h"

extern awesome_t globalconf;
extern bool running;

/** Draw a statusbar.
 * \param statusbar The statusbar to draw.
 */
static void
statusbar_draw(statusbar_t *statusbar)
{
    xcb_window_t rootwin;
    xcb_pixmap_t rootpix;
    widget_node_t *w;
    int left = 0, right = 0;
    char *data;
    xcb_get_property_reply_t *prop_r;
    xcb_get_property_cookie_t prop_c;
    area_t rectangle = { 0, 0, 0, 0, NULL, NULL }, rootsize;;
    xcb_atom_t rootpix_atom, pixmap_atom;
    xutil_intern_atom_request_t rootpix_atom_req, pixmap_atom_req;

    statusbar->need_update = false;

    if(!statusbar->position)
        return;

    /* Send requests needed for transparency */
    if(statusbar->colors.bg.alpha != 0xffff)
    {
        pixmap_atom_req = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "PIXMAP");
        rootpix_atom_req = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "_XROOTPMAP_ID");
    }

    rectangle.width = statusbar->width;
    rectangle.height = statusbar->height;

    if(statusbar->colors.bg.alpha != 0xffff)
    {
        rootwin = xcb_aux_get_screen(globalconf.connection, statusbar->phys_screen)->root;
        pixmap_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, pixmap_atom_req);
        rootpix_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, rootpix_atom_req);
        prop_c = xcb_get_property_unchecked(globalconf.connection, false, rootwin, rootpix_atom,
                                            pixmap_atom, 0, 1);
        if((prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL)))
        {
            if((data = xcb_get_property_value(prop_r)))
            {
               rootpix = *(xcb_pixmap_t *) data;
               switch(statusbar->position)
               {
                 case Left:
                   rootsize = get_display_area(statusbar->phys_screen, NULL, NULL);
                   draw_rotate(statusbar->ctx,
                               rootpix, statusbar->ctx->drawable,
                               rootsize.width, rootsize.height,
                               statusbar->width, statusbar->height,
                               M_PI_2,
                               statusbar->sw->geometry.y + statusbar->width,
                               - statusbar->sw->geometry.x);
                   break;
                 case Right:
                   rootsize = get_display_area(statusbar->phys_screen, NULL, NULL);
                   draw_rotate(statusbar->ctx,
                               rootpix, statusbar->ctx->drawable,
                               rootsize.width, rootsize.height,
                               statusbar->width, statusbar->height,
                               - M_PI_2,
                               - statusbar->sw->geometry.y,
                               statusbar->sw->geometry.x + statusbar->height);
                   break;
                 default:
                   xcb_copy_area(globalconf.connection, rootpix,
                                 statusbar->sw->drawable, statusbar->sw->gc,
                                 statusbar->sw->geometry.x, statusbar->sw->geometry.y,
                                 0, 0,
                                 statusbar->sw->geometry.width,
                                 statusbar->sw->geometry.height);
                   break;
               }
            }
            p_delete(&prop_r);
        }
    }

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
          draw_rotate(statusbar->ctx, statusbar->ctx->drawable, statusbar->sw->drawable,
                      statusbar->ctx->width, statusbar->ctx->height,
                      statusbar->ctx->height, statusbar->ctx->width,
                      M_PI_2, statusbar->height, 0);
          break;
        case Left:
          draw_rotate(statusbar->ctx, statusbar->ctx->drawable, statusbar->sw->drawable,
                      statusbar->ctx->width, statusbar->ctx->height,
                      statusbar->ctx->height, statusbar->ctx->width,
                      - M_PI_2, 0, statusbar->width);
          break;
        default:
          break;
    }

    simplewindow_refresh_drawable(statusbar->sw);
    xcb_aux_sync(globalconf.connection);
}

/** Statusbar refresh function.
 * \param p A pointer to a statusbar_t.
 * \return Return NULL.
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
    xcb_drawable_t dw;
    xcb_screen_t *s = NULL;
    bool ignore = false;

    globalconf.screens[statusbar->screen].need_arrange = true;

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

    xcb_map_window(globalconf.connection, statusbar->sw->window);

    /* Set need update */
    statusbar->need_update = true;
}

/** Check for statusbar equality.
 * \param A statusbar.
 * \return True if statusbar are equals, false otherwise.
 */
static int
luaA_statusbar_eq(lua_State *L)
{
    statusbar_t **t1 = luaL_checkudata(L, 1, "statusbar");
    statusbar_t **t2 = luaL_checkudata(L, 2, "statusbar");
    lua_pushboolean(L, (*t1 == *t2));
    return 1;
}

/** Set the statusbar position.
 * \param A position: left, right, top, bottom or off.
 */
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

/** Get the statusbar position.
 * \return The statusbar position.
 */
static int
luaA_statusbar_position_get(lua_State *L)
{
    statusbar_t **sb = luaL_checkudata(L, 1, "statusbar");
    lua_pushstring(L, position_to_str((*sb)->position));
    return 1;
}

/** Set the statusbar alignment on screen.
 * \param An alignment: right, left or center.
 */
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

/** Convert a statusbar to a printable string.
 */
static int
luaA_statusbar_tostring(lua_State *L)
{
    statusbar_t **p = luaL_checkudata(L, 1, "statusbar");
    lua_pushfstring(L, "[statusbar udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

/** Add a widget to a statusbar.
 * \param A widget.
 */
static int
luaA_statusbar_widget_add(lua_State *L)
{
    statusbar_t **sb = luaL_checkudata(L, 1, "statusbar");
    widget_t **widget = luaL_checkudata(L, 2, "widget");
    widget_node_t *w = p_new(widget_node_t, 1);

    (*sb)->need_update = true;
    w->widget = *widget;
    widget_node_list_append(&(*sb)->widgets, w);
    widget_ref(widget);

    return 0;
}

/** Add the statusbar on a screen.
 * \param A screen number.
 */
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
    (*sb)->phys_screen = screen_virttophys(screen);

    statusbar_list_append(&globalconf.screens[screen].statusbar, *sb);
    statusbar_ref(sb);

    /* All the other statusbar and ourselves need to be repositionned */
    for(s = globalconf.screens[screen].statusbar; s; s = s->next)
        statusbar_position_update(s, s->position);

    return 0;
}

/** Remove the statusbar its screen.
 */
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

/** Create a new statusbar.
 * \param A table with at least a name attribute. Optionnaly defined values are:
 * position, align, fg, bg, width and height.
 * \return A brand new statusbar.
 */
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
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &(*sb)->colors.fg);
    else
        (*sb)->colors.fg = globalconf.colors.fg;

    lua_getfield(L, 1, "bg");
    if((color = luaL_optstring(L, -1, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
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

/** Handle statusbar garbage collection.
 */
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
    { "position_get", luaA_statusbar_position_get },
    { "align_set", luaA_statusbar_align_set },
    { "add", luaA_statusbar_add },
    { "remove", luaA_statusbar_remove },
    { "__gc", luaA_statusbar_gc },
    { "__eq", luaA_statusbar_eq },
    { "__tostring", luaA_statusbar_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
