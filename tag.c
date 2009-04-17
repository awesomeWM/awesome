/*
 * tag.c - tag management
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

#include "screen.h"
#include "tag.h"
#include "client.h"
#include "ewmh.h"
#include "widget.h"

DO_LUA_TOSTRING(tag_t, tag, "tag")

void
tag_unref_simplified(tag_t **tag)
{
    tag_unref(globalconf.L, *tag);
}

/** Garbage collect a tag.
 * \param L The Lua VM state.
 * \return 0.
 */
static int
luaA_tag_gc(lua_State *L)
{
    tag_t *tag = luaL_checkudata(L, 1, "tag");
    client_array_wipe(&tag->clients);
    p_delete(&tag->name);
    return 0;
}

/** View or unview a tag.
 * \param tag the tag
 * \param view set visible or not
 */
static void
tag_view(tag_t *tag, bool view)
{
    tag->selected = view;
    ewmh_update_net_current_desktop(screen_virttophys(tag->screen->index));
    tag->screen->need_arrange = true;
}

/** Append a tag which on top of the stack to a screen.
 * \param screen the screen id
 */
void
tag_append_to_screen(screen_t *s)
{
    int phys_screen = screen_virttophys(s->index);
    tag_t *tag = tag_ref(globalconf.L);

    tag->screen = s;
    tag_array_append(&s->tags, tag);
    ewmh_update_net_numbers_of_desktop(phys_screen);
    ewmh_update_net_desktop_names(phys_screen);
    ewmh_update_workarea(phys_screen);

    /* call hook */
    if(globalconf.hooks.tags != LUA_REFNIL)
    {
        lua_pushnumber(globalconf.L, s->index + 1);
        tag_push(globalconf.L, tag);
        lua_pushliteral(globalconf.L, "add");
        luaA_dofunction(globalconf.L, globalconf.hooks.tags, 3, 0);
    }
}

/** Remove a tag from screen. Tag must be on a screen and have no clients.
 * \param tag The tag to remove.
 */
static void
tag_remove_from_screen(tag_t *tag)
{
    int phys_screen = screen_virttophys(tag->screen->index);
    tag_array_t *tags = &tag->screen->tags;

    for(int i = 0; i < tags->len; i++)
        if(tags->tab[i] == tag)
        {
            tag_array_take(tags, i);
            break;
        }
    ewmh_update_net_numbers_of_desktop(phys_screen);
    ewmh_update_net_desktop_names(phys_screen);
    ewmh_update_workarea(phys_screen);
    tag->screen = NULL;

    /* call hook */
    if(globalconf.hooks.tags != LUA_REFNIL)
    {
        lua_pushnumber(globalconf.L, tag->screen->index + 1);
        tag_push(globalconf.L, tag);
        lua_pushliteral(globalconf.L, "remove");
        luaA_dofunction(globalconf.L, globalconf.hooks.tags, 3, 0);
    }

    tag_unref(globalconf.L, tag);
}

/** Tag a client with the tag on top of the stack.
 * \param c the client to tag
 */
void
tag_client(client_t *c)
{
    tag_t *t = tag_ref(globalconf.L);

    /* don't tag twice */
    if(is_client_tagged(c, t))
        return;

    client_array_append(&t->clients, c);
    ewmh_client_update_desktop(c);
    client_need_arrange(c);
    /* call hook */
    if(globalconf.hooks.tagged != LUA_REFNIL)
    {
        client_push(globalconf.L, c);
        tag_push(globalconf.L, t);
        luaA_dofunction(globalconf.L, globalconf.hooks.tagged, 2, 0);
    }
}

/** Untag a client with specified tag.
 * \param c the client to tag
 * \param t the tag to tag the client with
 */
void
untag_client(client_t *c, tag_t *t)
{
    for(int i = 0; i < t->clients.len; i++)
        if(t->clients.tab[i] == c)
        {
            client_need_arrange(c);
            client_array_take(&t->clients, i);
            ewmh_client_update_desktop(c);
            /* call hook */
            if(globalconf.hooks.tagged != LUA_REFNIL)
            {
                client_push(globalconf.L, c);
                tag_push(globalconf.L, t);
                luaA_dofunction(globalconf.L, globalconf.hooks.tagged, 2, 0);
            }
            tag_unref(globalconf.L, t);
            return;
        }
}

/** Check if a client is tagged with the specified tag.
 * \param c the client
 * \param t the tag
 * \return true if the client is tagged with the tag, false otherwise.
 */
bool
is_client_tagged(client_t *c, tag_t *t)
{
    for(int i = 0; i < t->clients.len; i++)
        if(t->clients.tab[i] == c)
            return true;

    return false;
}

/** Get the current tags for the specified screen.
 * Returned pointer must be p_delete'd after.
 * \param screen Screen.
 * \return A double pointer of tag list finished with a NULL element.
 */
tag_t **
tags_get_current(screen_t *screen)
{
    tag_t **out = p_new(tag_t *, screen->tags.len + 1);
    int n = 0;

    foreach(tag, screen->tags)
        if((*tag)->selected)
            out[n++] = *tag;

    return out;
}


/** Set a tag to be the only one viewed.
 * \param target the tag to see
 */
static void
tag_view_only(tag_t *target)
{
    if(target)
        foreach(tag, target->screen->tags)
            tag_view(*tag, *tag == target);
}

/** View only a tag, selected by its index.
 * \param screen Screen.
 * \param dindex The index.
 */
void
tag_view_only_byindex(screen_t *screen, int dindex)
{
    tag_array_t *tags = &screen->tags;

    if(dindex < 0 || dindex >= tags->len)
        return;
    tag_view_only(tags->tab[dindex]);
}

/** Create a new tag.
 * \param L The Lua VM state.
 * \luastack
 * \lparam A name.
 * \lreturn A new tag object.
 */
static int
luaA_tag_new(lua_State *L)
{
    size_t len;
    const char *name = luaL_checklstring(L, 2, &len);
    tag_t *tag = tag_new(globalconf.L);

    a_iso2utf8(name, len, &tag->name, NULL);

    return 1;
}

/** Get or set the clients attached to this tag.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam None or a table of clients to set.
 * \lreturn A table with the clients attached to this tags.
 */
static int
luaA_tag_clients(lua_State *L)
{
    tag_t *tag = luaL_checkudata(L, 1, "tag");
    client_array_t *clients = &tag->clients;
    int i;

    if(lua_gettop(L) == 2)
    {
        luaA_checktable(L, 2);
        foreach(c, tag->clients)
            untag_client(*c, tag);
        lua_pushnil(L);
        while(lua_next(L, 2))
        {
            client_t *c = luaA_client_checkudata(L, -1);
            /* push tag on top of the stack */
            lua_pushvalue(L, 1);
            tag_client(c);
            lua_pop(L, 1);
        }
    }

    lua_createtable(L, clients->len, 0);
    for(i = 0; i < clients->len; i++)
    {
        client_push(L, clients->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/** Tag object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield name Tag name.
 * \lfield screen Screen number of the tag.
 * \lfield layout Tag layout.
 * \lfield selected True if the client is selected to be viewed.
 */
static int
luaA_tag_index(lua_State *L)
{
    size_t len;
    tag_t *tag = luaL_checkudata(L, 1, "tag");
    const char *attr;

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_NAME:
        lua_pushstring(L, tag->name);
        break;
      case A_TK_SCREEN:
        if(!tag->screen)
            return 0;
        lua_pushnumber(L, tag->screen->index + 1);
        break;
      case A_TK_SELECTED:
        lua_pushboolean(L, tag->selected);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Tag newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_tag_newindex(lua_State *L)
{
    size_t len;
    tag_t *tag = luaL_checkudata(L, 1, "tag");
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
        int screen;

      case A_TK_NAME:
        {
            const char *buf = luaL_checklstring(L, 3, &len);
            p_delete(&tag->name);
            a_iso2utf8(buf, len, &tag->name, NULL);
        }
        break;
      case A_TK_SCREEN:
        if(!lua_isnil(L, 3))
        {
            screen = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(screen);
        }
        else
            screen = -1;

        if(tag->screen)
            tag_remove_from_screen(tag);

        if(screen != -1)
        {
            /* push tag on top of the stack */
            lua_pushvalue(L, 1);
            tag_append_to_screen(&globalconf.screens.tab[screen]);
        }
        break;
      case A_TK_SELECTED:
        if(tag->screen)
            tag_view(tag, luaA_checkboolean(L, 3));
        return 0;
      default:
        return 0;
    }

    if(tag->screen && tag->selected)
        tag->screen->need_arrange = true;

    return 0;
}

const struct luaL_reg awesome_tag_methods[] =
{
    { "__call", luaA_tag_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_tag_meta[] =
{
    { "clients", luaA_tag_clients },
    { "__index", luaA_tag_index },
    { "__newindex", luaA_tag_newindex },
    { "__gc", luaA_tag_gc },
    { "__tostring", luaA_tag_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
