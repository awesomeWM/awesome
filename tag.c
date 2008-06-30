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
 * \param name tag name
 * \param layout layout to use
 * \param mwfact master width factor
 * \param nmaster number of master windows
 * \param ncol number of columns for slaves windows
 * \return a new tag with all these parameters
 */
tag_t *
tag_new(const char *name, layout_t *layout, double mwfact, int nmaster, int ncol)
{
    tag_t *tag;

    tag = p_new(tag_t, 1);
    a_iso2utf8(name, &tag->name);
    tag->layout = layout;

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
tag_append_to_screen(tag_t *tag, int screen)
{
    int phys_screen = screen_virttophys(screen);

    tag->screen = screen;
    tag_array_append(&globalconf.screens[screen].tags, tag_ref(&tag));
    ewmh_update_net_numbers_of_desktop(phys_screen);
    ewmh_update_net_desktop_names(phys_screen);
    ewmh_update_workarea(phys_screen);
    widget_invalidate_cache(screen, WIDGET_CACHE_TAGS);
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
    if (target) {
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

/** Add a tag to a screen.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam A screen number.
 */
static int
luaA_tag_add(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    int screen = luaL_checknumber(L, 2) - 1;
    luaA_checkscreen(screen);

    for(int i = 0; i < globalconf.screens_info->nscreen; i++)
    {
        tag_array_t *tags = &globalconf.screens[i].tags;
        for(int j = 0; j < tags->len; j++)
        {
            tag_t *t = tags->tab[j];
            if(*tag == t)
                luaL_error(L, "tag already on screen %d", i + 1);
            else if(t->screen == screen && !a_strcmp((*tag)->name, t->name))
                luaL_error(L, "a tag with the name `%s' is already on screen %d", t->name, i + 1);
        }
    }

    (*tag)->screen = screen;
    tag_append_to_screen(*tag, screen);
    return 0;
}

/** Get all tags from a screen.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A screen number.
 * \lreturn A table with all tags from the screen specified, indexed by tag name.
 */
static int
luaA_tag_get(lua_State *L)
{
    int screen = luaL_checknumber(L, 1) - 1;
    tag_array_t *tags = &globalconf.screens[screen].tags;

    luaA_checkscreen(screen);

    lua_newtable(L);

    for(int i = 0; i < tags->len; i++)
    {
        luaA_tag_userdata_new(L, tags->tab[i]);
        lua_setfield(L, -2, tags->tab[i]->name);
    }

    return 1;
}

/** Get all tags from a screen.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A screen number.
 * \lreturn A table with all tags from the screen specified, ordered and indexed
 * by number.
 */
static int
luaA_tag_geti(lua_State *L)
{
    int screen = luaL_checknumber(L, 1) - 1;
    tag_array_t *tags = &globalconf.screens[screen].tags;

    luaA_checkscreen(screen);

    lua_newtable(L);

    for(int i = 0; i < tags->len; i++)
    {
        luaA_tag_userdata_new(L, tags->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

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
    lay = luaA_getopt_string(L, 2, "layout", "tile");

    layout = name_func_lookup(lay, LayoutList);

    tag = tag_new(name,
                  layout,
                  mwfact, nmaster, ncol);

    return luaA_tag_userdata_new(L, tag);
}

/** Add or remove a tag from the current view.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam A boolean value, true to view tag, false otherwise.
 */
static int
luaA_tag_view(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    bool view = luaA_checkboolean(L, 2);
    tag_view(*tag, view);
    return 0;
}

/** Get the tag selection attribute.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn True if the tag is viewed, false otherwise.
 */
static int
luaA_tag_isselected(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    lua_pushboolean(L, (*tag)->selected);
    return 1;
}

/** Set the tag master width factor. This value is used in various layouts to
 * determine the size of the master window.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam The master width ratio value, between 0 and 1.
 */
static int
luaA_tag_mwfact_set(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    double mwfact = luaL_checknumber(L, 2);

    if(mwfact < 1 && mwfact > 0)
    {
        (*tag)->mwfact = mwfact;
        globalconf.screens[(*tag)->screen].need_arrange = true;
    }
    else
        luaL_error(L, "bad value, must be between 0 and 1");

    return 0;
}

/** Get the tag master width factor.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn The master width ratio value.
 */
static int
luaA_tag_mwfact_get(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    lua_pushnumber(L, (*tag)->mwfact);
    return 1;
}

/** Set the number of columns. This is used in various layouts to set the number
 * of columns used to display non-master windows.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam The number of columns, at least 1.
 */
static int
luaA_tag_ncol_set(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    int ncol = luaL_checknumber(L, 2);

    if(ncol >= 1)
    {
        (*tag)->ncol = ncol;
        globalconf.screens[(*tag)->screen].need_arrange = true;
    }
    else
        luaL_error(L, "bad value, must be greater than 1");

    return 0;
}

/** Get the number of columns used to display non-master windows on this tag.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn The number of column.
 */
static int
luaA_tag_ncol_get(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    lua_pushnumber(L, (*tag)->ncol);
    return 1;
}

/** Set the number of master windows. This is used in various layouts to
 * determine how many windows are in the master area.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam The number of master windows.
 */
static int
luaA_tag_nmaster_set(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    int nmaster = luaL_checknumber(L, 2);

    if(nmaster >= 0)
    {
        (*tag)->nmaster = nmaster;
        globalconf.screens[(*tag)->screen].need_arrange = true;
    }
    else
        luaL_error(L, "bad value, must be greater than 0");

    return 0;
}

/** Get the number of master windows of the tag.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn The number of master windows.
 */
static int
luaA_tag_nmaster_get(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    lua_pushnumber(L, (*tag)->nmaster);
    return 1;
}

/** Get the tag name.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn The tag name.
 */
static int
luaA_tag_name_get(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    lua_pushstring(L, (*tag)->name);
    return 1;
}

/** Set the tag name.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam A string with the new tag name.
 */
static int
luaA_tag_name_set(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    const char *name = luaL_checkstring(L, 2);
    p_delete(&(*tag)->name);
    a_iso2utf8(name, &(*tag)->name);
    return 0;
}

/** Get the layout of the tag.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lreturn The layout name.
 */
static int
luaA_tag_layout_get(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    const char *name = a_strdup(name_func_rlookup((*tag)->layout, LayoutList));
    lua_pushstring(L, name);
    return 1;
}

/** Set the layout of the tag.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A tag.
 * \lparam A layout name.
 */
static int
luaA_tag_layout_set(lua_State *L)
{
    tag_t **tag = luaA_checkudata(L, 1, "tag");
    const char *name = luaL_checkstring(L, 2);
    layout_t *l = name_func_lookup(name, LayoutList);

    if(l)
    {
        (*tag)->layout = l;
        globalconf.screens[(*tag)->screen].need_arrange = true;
    }
    else
        luaL_error(L, "unknown layout: %s", name);

    return 0;

}

const struct luaL_reg awesome_tag_methods[] =
{
    { "__call", luaA_tag_new },
    { "get", luaA_tag_get },
    { "geti", luaA_tag_geti },
    { NULL, NULL }
};
const struct luaL_reg awesome_tag_meta[] =
{
    { "add", luaA_tag_add },
    { "view", luaA_tag_view },
    { "isselected", luaA_tag_isselected },
    { "mwfact_set", luaA_tag_mwfact_set },
    { "mwfact_get", luaA_tag_mwfact_get },
    { "ncol_set", luaA_tag_ncol_set },
    { "ncol_get", luaA_tag_ncol_get },
    { "nmaster_set", luaA_tag_nmaster_set },
    { "nmaster_get", luaA_tag_nmaster_get },
    { "name_get", luaA_tag_name_get },
    { "name_set", luaA_tag_name_set },
    { "layout_get", luaA_tag_layout_get },
    { "layout_set", luaA_tag_layout_set },
    { "__eq", luaA_tag_eq },
    { "__gc", luaA_tag_gc },
    { "__tostring", luaA_tag_tostring },
    { NULL, NULL },
};
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
