/*
 * screen.c - screen management
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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "screen.h"
#include "ewmh.h"
#include "tag.h"
#include "client.h"
#include "layouts/tile.h"

extern awesome_t globalconf;

/** Get screens info.
 * \param screen Screen number.
 * \param statusbar Statusbar list to remove.
 * \param padding Padding.
 * \return The screen area.
 */
area_t
screen_area_get(area_t *geometry, statusbar_t *statusbar, padding_t *padding)
{
    area_t area = *geometry;
    statusbar_t *sb;

    /* make padding corrections */
    if(padding)
    {
        area.x += padding->left;
        area.y += padding->top;
        area.width -= padding->left + padding->right;
        area.height -= padding->top + padding->bottom;
    }

    for(sb = statusbar; sb; sb = sb->next)
        switch(sb->position)
        {
          case Top:
            area.y += sb->height;
          case Bottom:
            area.height -= sb->height;
            break;
          case Left:
            area.x += sb->height;
          case Right:
            area.width -= sb->height;
            break;
          default:
            break;
        }

    return area;
}

/** Get display info.
 * \param phys_screen Physical screen number.
 * \param statusbar The statusbars.
 * \param padding Padding.
 * \return The display area.
 */
area_t
display_area_get(int phys_screen, statusbar_t *statusbar, padding_t *padding)
{
    area_t area = { 0, 0, 0, 0 };
    statusbar_t *sb;
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);

    area.width = s->width_in_pixels;
    area.height = s->height_in_pixels;

    for(sb = statusbar; sb; sb = sb->next)
    {
        area.y += sb->position == Top ? sb->height : 0;
        area.height -= (sb->position == Top || sb->position == Bottom) ? sb->height : 0;
    }

    /* make padding corrections */
    if(padding)
    {
            area.x += padding->left;
            area.y += padding->top;
            area.width -= padding->left + padding->right;
            area.height -= padding->top + padding->bottom;
    }
    return area;
}

/** This returns the real X screen number for a logical
 * screen if Xinerama is active.
 * \param screen The logical screen.
 * \return The X screen.
 */
int
screen_virttophys(int screen)
{
    if(globalconf.screens_info->xinerama_is_active)
        return globalconf.default_screen;
    return screen;
}

/** Move a client to a virtual screen.
 * \param c The client to move.
 * \param new_screen The destinatiuon screen number.
 * \param doresize Set to true if we also move the client to the new x and
 *        y of the new screen.
 */
void
screen_client_moveto(client_t *c, int new_screen, bool doresize)
{
    int i, old_screen = c->screen;
    tag_array_t *old_tags = &globalconf.screens[old_screen].tags,
                *new_tags = &globalconf.screens[new_screen].tags;
    area_t from, to;

    c->screen = new_screen;

    /* remove old tags */
    for(i = 0; i < old_tags->len; i++)
        untag_client(c, old_tags->tab[i]);

    /* add new tags */
    for(i = 0; i < new_tags->len; i++)
        if(new_tags->tab[i]->selected)
            tag_client(c, new_tags->tab[i]);

    /* resize the windows if it's floating */
    if(doresize && old_screen != c->screen)
    {
        area_t new_geometry, new_f_geometry;
        new_f_geometry = c->f_geometry;

        to = screen_area_get(&globalconf.screens[c->screen].geometry,
                             NULL, NULL);
        from = screen_area_get(&globalconf.screens[old_screen].geometry,
                               NULL, NULL);

        /* compute new coords in new screen */
        new_f_geometry.x = (c->f_geometry.x - from.x) + to.x;
        new_f_geometry.y = (c->f_geometry.y - from.y) + to.y;

        /* check that new coords are still in the screen */
        if(new_f_geometry.width > to.width)
            new_f_geometry.width = to.width;
        if(new_f_geometry.height > to.height)
            new_f_geometry.height = to.height;
        if(new_f_geometry.x + new_f_geometry.width >= to.x + to.width)
            new_f_geometry.x = to.x + to.width - new_f_geometry.width - 2 * c->border;
        if(new_f_geometry.y + new_f_geometry.height >= to.y + to.height)
            new_f_geometry.y = to.y + to.height - new_f_geometry.height - 2 * c->border;

        if(c->ismax)
        {
            new_geometry = c->geometry;

            /* compute new coords in new screen */
            new_geometry.x = (c->geometry.x - from.x) + to.x;
            new_geometry.y = (c->geometry.y - from.y) + to.y;

            /* check that new coords are still in the screen */
            if(new_geometry.width > to.width)
                new_geometry.width = to.width;
            if(new_geometry.height > to.height)
                new_geometry.height = to.height;
            if(new_geometry.x + new_geometry.width >= to.x + to.width)
                new_geometry.x = to.x + to.width - new_geometry.width - 2 * c->border;
            if(new_geometry.y + new_geometry.height >= to.y + to.height)
                new_geometry.y = to.y + to.height - new_geometry.height - 2 * c->border;

            /* compute new coords for max in new screen */
            c->m_geometry.x = (c->m_geometry.x - from.x) + to.x;
            c->m_geometry.y = (c->m_geometry.y - from.y) + to.y;

            /* check that new coords are still in the screen */
            if(c->m_geometry.width > to.width)
                c->m_geometry.width = to.width;
            if(c->m_geometry.height > to.height)
                c->m_geometry.height = to.height;
            if(c->m_geometry.x + c->m_geometry.width >= to.x + to.width)
                c->m_geometry.x = to.x + to.width - c->m_geometry.width - 2 * c->border;
            if(c->m_geometry.y + c->m_geometry.height >= to.y + to.height)
                c->m_geometry.y = to.y + to.height - c->m_geometry.height - 2 * c->border;

            client_resize(c, new_geometry, false);
        }
        /* if floating, move to this new coords */
        else if(c->isfloating)
            client_resize(c, new_f_geometry, false);
        /* otherwise just register them */
        else
        {
            c->f_geometry = new_f_geometry;
            globalconf.screens[old_screen].need_arrange = true;
            globalconf.screens[c->screen].need_arrange = true;
        }
    }
}

/** Screen module.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield number The screen number, to get a screen.
 */
static int
luaA_screen_module_index(lua_State *L)
{
    int screen = luaL_checknumber(L, 2) - 1;

    luaA_checkscreen(screen);
    lua_pushlightuserdata(L, &globalconf.screens[screen]);
    return luaA_settype(L, "screen");
}

/** Get or set screen tags.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam None or a table of tags to set to the screen.
 * The table must contains at least one tag.
 * \return A table with all screen tags.
 */
static int
luaA_screen_tags(lua_State *L)
{
    int i;
    screen_t *s = lua_touserdata(L, 1);

    if(!s)
        luaL_typerror(L, 1, "screen");

    if(lua_gettop(L) == 2)
    {
        tag_t **tag;

        luaA_checktable(L, 2);

        /* remove current tags */
        for(i = 0; i < s->tags.len; i++)
            s->tags.tab[i]->screen = SCREEN_UNDEF;

        tag_array_wipe(&s->tags);
        tag_array_init(&s->tags);

        s->need_arrange = true;

        /* push new tags */
        lua_pushnil(L);
        while(lua_next(L, 2))
        {
            tag = luaA_checkudata(L, -1, "tag");
            tag_append_to_screen(*tag, s);
            lua_pop(L, 1);
        }

        /* check there's at least one tag! */
        if(!s->tags.len)
        {
            tag_append_to_screen(tag_new("default", sizeof("default") - 1, layout_tile, 0.5, 1, 0), s);
            luaL_error(L, "no tag were added on screen %d, taking last resort action and adding default tag\n", s->index);
        }
    }
    else
    {
        lua_newtable(L);
        for(i = 0; i < s->tags.len; i++)
        {
            luaA_tag_userdata_new(L, s->tags.tab[i]);
            lua_rawseti(L, -2, i + 1);
        }
    }

    return 1;
}

/** A screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield padding The screen padding. A table with top, right, left and bottom
 * keys and values in pixel.
 * \lfield coords The screen coordinates. Immutable.
 * \lfield workarea The screen workarea, i.e. without statusbar.
 */
static int
luaA_screen_index(lua_State *L)
{
    size_t len;
    const char *buf;
    screen_t *s;
    area_t g;

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    buf = luaL_checklstring(L, 2, &len);
    s = lua_touserdata(L, 1);

    switch(a_tokenize(buf, len))
    {
      case A_TK_PADDING:
        lua_newtable(L);
        lua_pushnumber(L, s->padding.right);
        lua_setfield(L, -2, "right");
        lua_pushnumber(L, s->padding.left);
        lua_setfield(L, -2, "left");
        lua_pushnumber(L, s->padding.top);
        lua_setfield(L, -2, "top");
        lua_pushnumber(L, s->padding.bottom);
        lua_setfield(L, -2, "bottom");
        break;
      case A_TK_COORDS:
        /* \todo lua_pushgeometry() ? */
        lua_newtable(L);
        lua_pushnumber(L, s->geometry.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, s->geometry.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, s->geometry.width);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, s->geometry.height);
        lua_setfield(L, -2, "height");
        break;
      case A_TK_WORKAREA:
        g = screen_area_get(&s->geometry, s->statusbar, &s->padding);
        lua_newtable(L);
        lua_pushnumber(L, g.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, g.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, g.width);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, g.height);
        lua_setfield(L, -2, "height");
        break;
      default:
        return 0;
    }

    return 1;
}

static int
luaA_screen_newindex(lua_State *L)
{
    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);
    screen_t *s;

    s = lua_touserdata(L, 1);

    switch(a_tokenize(buf, len))
    {
      case A_TK_PADDING:
        luaA_checktable(L, 3);
        s->padding.right = luaA_getopt_number(L, 2, "right", 0);
        s->padding.left = luaA_getopt_number(L, 2, "left", 0);
        s->padding.top = luaA_getopt_number(L, 2, "top", 0);
        s->padding.bottom = luaA_getopt_number(L, 2, "bottom", 0);

        ewmh_update_workarea(screen_virttophys(s->index));

        break;
      default:
        return 0;
    }

    return 0;
}

/** Get the screen count.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 *
 * \luastack
 * \lreturn The screen count, at least 1.
 */
static int
luaA_screen_count(lua_State *L)
{
    lua_pushnumber(L, globalconf.screens_info->nscreen);
    return 1;
}

const struct luaL_reg awesome_screen_methods[] =
{
    { "count", luaA_screen_count },
    { "__index", luaA_screen_module_index },
    { NULL, NULL }
};

const struct luaL_reg awesome_screen_meta[] =
{
    { "tags", luaA_screen_tags },
    { "__index", luaA_screen_index },
    { "__newindex", luaA_screen_newindex },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
