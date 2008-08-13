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

#include "layouts/magnifier.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/floating.h"
#include "layouts/fibonacci.h"

#include "layoutgen.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, tag_t, tag, "tag", tag_ref)
DO_LUA_GC(tag_t, tag, "tag", tag_unref)
DO_LUA_EQ(tag_t, tag, "tag")

/** View or unview a tag.
 * \param tag the tag
 * \param view set visible or not
 */
static void
tag_view(tag_t *tag, bool view)
{
    tag->selected = view;
    ewmh_update_net_current_desktop(screen_virttophys(tag->screen));
    widget_invalidate_cache(tag->screen, WIDGET_CACHE_TAGS);
    globalconf.screens[tag->screen].need_arrange = true;
}

/** Create a new tag. Parameters values are checked.
 * \param name Tag name.
 * \param len Tag name length.
 * \param layout Layout to use.
 * \param mwfact Master width factor.
 * \param nmaster Number of master windows.
 * \param ncol Number of columns for slaves windows.
 * \return A new tag with all these parameters.
 */
tag_t *
tag_new(const char *name, ssize_t len, layout_t *layout, double mwfact, int nmaster, int ncol)
{
    tag_t *tag;

    tag = p_new(tag_t, 1);
    a_iso2utf8(&tag->name, name, len);
    tag->layout = layout;

    /* to avoid error */
    tag->screen = SCREEN_UNDEF;

    tag->mwfact = mwfact;
    if(tag->mwfact <= 0 || tag->mwfact >= 1)
        tag->mwfact = 0.5;

    if((tag->nmaster = nmaster) < 0)
        tag->nmaster = 1;

    if((tag->ncol = ncol) < 1)
        tag->ncol = 1;

    return tag;
}

/** Append a tag to a screen.
 * \param tag the tag to append
 * \param screen the screen id
 */
void
tag_append_to_screen(tag_t *tag, screen_t *s)
{
    int phys_screen = screen_virttophys(s->index);

    tag->screen = s->index;
    tag_array_append(&s->tags, tag_ref(&tag));
    ewmh_update_net_numbers_of_desktop(phys_screen);
    ewmh_update_net_desktop_names(phys_screen);
    ewmh_update_workarea(phys_screen);
    widget_invalidate_cache(s->index, WIDGET_CACHE_TAGS);
}

/** Remove a tag from screen. Tag must be on a screen and have no clients.
 * \param tag The tag to remove.
 */
static void
tag_remove_from_screen(tag_t *tag)
{
    int phys_screen = screen_virttophys(tag->screen);
    tag_array_t *tags = &globalconf.screens[tag->screen].tags;

    for(int i = 0; i < tags->len; i++)
        if(tags->tab[i] == tag)
        {
            tag_array_take(tags, i);
            break;
        }
    ewmh_update_net_numbers_of_desktop(phys_screen);
    ewmh_update_net_desktop_names(phys_screen);
    ewmh_update_workarea(phys_screen);
    widget_invalidate_cache(tag->screen, WIDGET_CACHE_TAGS);
    tag->screen = SCREEN_UNDEF;
    tag_unref(&tag);
}

/** Tag a client with specified tag.
 * \param c the client to tag
 * \param t the tag to tag the client with
 */
void
tag_client(client_t *c, tag_t *t)
{
    /* don't tag twice */
    if(is_client_tagged(c, t))
        return;

    tag_ref(&t);
    client_array_append(&t->clients, c);
    client_saveprops(c);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    globalconf.screens[c->screen].need_arrange = true;
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
            client_array_take(&t->clients, i);
            tag_unref(&t);
            client_saveprops(c);
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
            globalconf.screens[c->screen].need_arrange = true;
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
        if (t->clients.tab[i] == c)
            return true;

    return false;
}

/** Get the current tags for the specified screen.
 * Returned pointer must be p_delete'd after.
 * \param screen screen id
 * \return a double pointer of tag list finished with a NULL element
 */
tag_t **
tags_get_current(int screen)
{
    tag_array_t *tags = &globalconf.screens[screen].tags;
    tag_t **out = p_new(tag_t *, tags->len + 1);
    int n = 0;

    for(int i = 0; i < tags->len; i++)
        if(tags->tab[i]->selected)
            out[n++] = tags->tab[i];

    return out;
}


/** Set a tag to be the only one viewed.
 * \param target the tag to see
 */
static void
tag_view_only(tag_t *target)
{
    if (target)
    {
        tag_array_t *tags = &globalconf.screens[target->screen].tags;

        for(int i = 0; i < tags->len; i++)
            tag_view(tags->tab[i], tags->tab[i] == target);
    }
}

/** View only a tag, selected by its index.
 * \param screen screen id
 * \param dindex the index
 */
void
tag_view_only_byindex(int screen, int dindex)
{
    tag_array_t *tags = &globalconf.screens[screen].tags;

    if(dindex < 0 || dindex >= tags->len)
        return;
    tag_view_only(tags->tab[dindex]);
}

/** Convert a tag to a printable string.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn A string.
 */
static int
luaA_tag_tostring(lua_State *L)
{
    tag_t **p = luaA_checkudata(L, 1, "tag");
    lua_pushfstring(L, "[tag udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

/** Create a new tag.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with at least a name attribute.
 * Optional attributes are: mwfact, ncol, nmaster and layout.
 * \lreturn A new tag object.
 */
static int
luaA_tag_new(lua_State *L)
{
    size_t len;
    tag_t *tag;
    int ncol, nmaster;
    const char *name, *lay;
    double mwfact;
    layout_t *layout;

    luaA_checktable(L, 2);

    if(!(name = luaA_getopt_string(L, 2, "name", NULL)))
        luaL_error(L, "object tag must have a name");

    mwfact = luaA_getopt_number(L, 2, "mwfact", 0.5);
    ncol = luaA_getopt_number(L, 2, "ncol", 1);
    nmaster = luaA_getopt_number(L, 2, "nmaster", 1);
    lay = luaA_getopt_lstring(L, 2, "layout", "tile", &len);

    layout = name_func_lookup(lay, LayoutList);

    tag = tag_new(name, len,
                  layout,
                  mwfact, nmaster, ncol);

    return luaA_tag_userdata_new(L, tag);
}

/** Get or set the clients attached to this tag.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \param None or a table of clients to set.
 * \return A table with the clients attached to this tags.
 */
static int
luaA_tag_clients(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    int i;

    if(lua_gettop(L) == 2)
    {
        client_t **c;

        luaA_checktable(L, 2);
        for(i = 0; i < (*tag)->clients.len; i++)
            untag_client((*tag)->clients.tab[i], *tag);
        lua_pushnil(L);
        while(lua_next(L, 2))
        {
            c = luaA_checkudata(L, -1, "client");
            tag_client(*c, *tag);
            lua_pop(L, 1);
        }
    }
    else
    {
        client_array_t *clients = &(*tag)->clients;
        lua_newtable(L);
        for(i = 0; i < clients->len; i++)
        {
            luaA_client_userdata_new(L, clients->tab[i]);
            lua_rawseti(L, -2, i + 1);
        }
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
 * \lfield mwfact Master width factor.
 * \lfield nmaster Number of master windows.
 * \lfield ncol Number of column for slave windows.
 */
static int
luaA_tag_index(lua_State *L)
{
    size_t len;
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    const char *attr;

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_NAME:
        lua_pushstring(L, (*tag)->name);
        break;
      case A_TK_SCREEN:
        if((*tag)->screen == SCREEN_UNDEF)
            return 0;
        lua_pushnumber(L, (*tag)->screen + 1);
        break;
      case A_TK_LAYOUT:
        lua_pushstring(L, name_func_rlookup((*tag)->layout, LayoutList));
        break;
      case A_TK_SELECTED:
        lua_pushboolean(L, (*tag)->selected);
        break;
      case A_TK_MWFACT:
        lua_pushnumber(L, (*tag)->mwfact);
        break;
      case A_TK_NMASTER:
        lua_pushnumber(L, (*tag)->nmaster);
        break;
      case A_TK_NCOL:
        lua_pushnumber(L, (*tag)->ncol);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Get a screen by its name.
 * \param screen The screen to look into.
 * \param name The name.
 */
static tag_t *
tag_getbyname(int screen, const char *name)
{
    int i;
    tag_array_t *tags = &globalconf.screens[screen].tags;

    for(i = 0; i < tags->len; i++)
        if(!a_strcmp(name, tags->tab[i]->name))
            return tags->tab[i];
    return NULL;
}

/** Tag newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_tag_newindex(lua_State *L)
{
    size_t len;
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    double d;
    int i, screen;
    layout_t *l;

    switch(a_tokenize(attr, len))
    {
      case A_TK_NAME:
        buf = luaL_checklstring(L, 3, &len);
        if((*tag)->screen != SCREEN_UNDEF)
        {
            if(tag_getbyname((*tag)->screen, (*tag)->name) != *tag)
                luaL_error(L, "a tag with the name `%s' is already on screen %d",
                           buf, (*tag)->screen);
            widget_invalidate_cache((*tag)->screen, WIDGET_CACHE_TAGS);
        }
        p_delete(&(*tag)->name);
        a_iso2utf8(&(*tag)->name, buf, len);
        break;
      case A_TK_SCREEN:
        if(!lua_isnil(L, 3))
        {
            screen = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(screen);
        }
        else
            screen = SCREEN_UNDEF;

        if((*tag)->screen != SCREEN_UNDEF)
            tag_remove_from_screen(*tag);

        if(screen != SCREEN_UNDEF)
        {
            if(tag_getbyname(screen, (*tag)->name))
                luaL_error(L, "a tag with the name `%s' is already on screen %d",
                           (*tag)->name, screen);

            tag_append_to_screen(*tag, &globalconf.screens[screen]);
        }
        break;
      case A_TK_LAYOUT:
        buf = luaL_checkstring(L, 3);
        l = name_func_lookup(buf, LayoutList);
        if(l)
            (*tag)->layout = l;
        else
            luaL_error(L, "unknown layout: %s", buf);
        break;
      case A_TK_SELECTED:
        if((*tag)->screen != SCREEN_UNDEF)
            tag_view(*tag, luaA_checkboolean(L, 3));
        return 0;
      case A_TK_MWFACT:
        d = luaL_checknumber(L, 3);
        if(d > 0 && d < 1)
            (*tag)->mwfact = d;
        else
            luaL_error(L, "bad value, must be between 0 and 1");
        break;
      case A_TK_NMASTER:
        i = luaL_checknumber(L, 3);
        if(i >= 0)
            (*tag)->nmaster = i;
        else
            luaL_error(L, "bad value, must be greater than 0");
        break;
      case A_TK_NCOL:
        i = luaL_checknumber(L, 3);
        if(i >= 1)
            (*tag)->ncol = i;
        else
            luaL_error(L, "bad value, must be greater than 1");
        break;
      default:
        return 0;
    }

    if((*tag)->screen != SCREEN_UNDEF)
        globalconf.screens[(*tag)->screen].need_arrange = true;

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
    { "__eq", luaA_tag_eq },
    { "__gc", luaA_tag_gc },
    { "__tostring", luaA_tag_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
