/*
 * workspace.c - workspace management
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
#include "workspace.h"
#include "client.h"
#include "ewmh.h"
#include "client.h"
#include "focus.h"
#include "widget.h"

#include "layouts/magnifier.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/floating.h"
#include "layouts/fibonacci.h"

#include "layoutgen.h"

extern awesome_t globalconf;

/** View a workspace.
 * \param workspace The workspace.
 * \param screen The screen to view on.
 */
static void
workspace_view(workspace_t *workspace, int screen)
{
    int s;

    for(s = 0; s < globalconf.screens_info->nscreen; s++)
        /* The workspace is already viewed on `s': switch */
        if(globalconf.screens[s].workspace == workspace)
        {
            globalconf.screens[s].workspace = globalconf.screens[screen].workspace;
            globalconf.screens[s].workspace->need_arrange = true;
        }
    
    ewmh_update_net_current_desktop(screen_virttophys(screen));
    widget_invalidate_cache(WIDGET_CACHE_WORKSPACES);
    globalconf.screens[screen].workspace = workspace;
    workspace->need_arrange = true;
}

/** Get the display screen for a workspace.
 * \param ws A workspace.
 * \return A screen number.
 */
int
workspace_screen_get(workspace_t *ws)
{
    int screen = 0;

    for(; screen < globalconf.screens_info->nscreen; screen++)
        if(globalconf.screens[screen].workspace == ws)
            return screen;
    return -1;
}

/** Create a new workspace. Parameteres values are checked.
 * \param name workspace name
 * \param layout layout to use
 * \param mwfact master width factor
 * \param nmaster number of master windows
 * \param ncol number of columns for slaves windows
 * \return a new workspace with all these parameters
 */
workspace_t *
workspace_new(const char *name, layout_t *layout, double mwfact, int nmaster, int ncol)
{
    workspace_t *workspace;

    workspace = p_new(workspace_t, 1);
    workspace->name = a_strdup(name);
    workspace->layout = layout;

    workspace->mwfact = mwfact;
    if(workspace->mwfact <= 0 || workspace->mwfact >= 1)
        workspace->mwfact = 0.5;

    if((workspace->nmaster = nmaster) < 0)
        workspace->nmaster = 1;

    if((workspace->ncol = ncol) < 1)
        workspace->ncol = 1;

    return workspace;
}

static workspace_client_node_t *
workspace_client_getnode(client_t *c)
{
    workspace_client_node_t *wc = NULL;
    for(wc = globalconf.wclink; wc && wc->client != c; wc = wc->next);
    return wc;
}

workspace_t *
workspace_client_get(client_t *c)
{
    workspace_client_node_t *wc = workspace_client_getnode(c);
    if(wc && wc->client == c)
        return wc->workspace;
    return NULL;
}

/** Remove the client from its workspace.
 * \param c The client
 */
void
workspace_client_remove(client_t *c)
{
    workspace_client_node_t *wc = workspace_client_getnode(c);

    if(wc && wc->client == c)
    {
        workspace_client_node_list_detach(&globalconf.wclink, wc);
        client_saveprops(c);
        widget_invalidate_cache(WIDGET_CACHE_CLIENTS);
        wc->workspace->need_arrange = true;
        p_delete(&wc);
    }
}

/** Put a client in the specified workspace.
 * \param c The client.
 * \param t The workspace.
 */
void
workspace_client_set(client_t *c, workspace_t *w)
{
    workspace_client_node_t *wc;

    if(!(wc = workspace_client_getnode(c)))
    {
        wc = p_new(workspace_client_node_t, 1);
        wc->client = c;
        workspace_client_node_list_push(&globalconf.wclink, wc);
    }
    wc->workspace = w;
    client_saveprops(c);
    widget_invalidate_cache(WIDGET_CACHE_CLIENTS);
    w->need_arrange = true;
}

/** Check for workspace equality.
 * \param Another workspace.
 * \return True if workspaces are equals.
 */
static int
luaA_workspace_eq(lua_State *L)
{
    workspace_t **t1 = luaA_checkudata(L, 1, "workspace");
    workspace_t **t2 = luaA_checkudata(L, 2, "workspace");
    lua_pushboolean(L, (*t1 == *t2));
    return 1;
}

/** Convert a workspace to a printable string.
 * \return A string.
 */
static int
luaA_workspace_tostring(lua_State *L)
{
    workspace_t **p = luaA_checkudata(L, 1, "workspace");
    lua_pushfstring(L, "[workspace udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

/** Add a workspace.
 */
static int
luaA_workspace_add(lua_State *L)
{
    workspace_t *t, **workspace = luaA_checkudata(L, 1, "workspace");

    for(t = globalconf.workspaces; t; t = t->next)
        if(*workspace == t)
            luaL_error(L, "workspace already added");

    workspace_list_append(&globalconf.workspaces, *workspace);
    workspace_ref(workspace);
    /* \todo do it for all screens */
    ewmh_update_net_numbers_of_desktop(globalconf.default_screen);
    ewmh_update_net_desktop_names(globalconf.default_screen);
    widget_invalidate_cache(WIDGET_CACHE_WORKSPACES);
    return 0;
}

/** Get all workspaces.
 * \return A table with all workspaces.
 */
static int
luaA_workspace_get(lua_State *L)
{
    workspace_t *workspace;
    int i = 1;

    lua_newtable(L);

    for(workspace = globalconf.workspaces; workspace; workspace = workspace->next)
    {
        luaA_workspace_userdata_new(workspace);
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/** Create a new workspace.
 * \param A table with at least a name attribute.
 * Optionnal attributes are: mwfact, ncol, nmaster and layout.
 * \return A new workspace object.
 */
static int
luaA_workspace_new(lua_State *L)
{
    workspace_t *workspace;
    int ncol, nmaster;
    const char *name, *lay;
    double mwfact;
    layout_t *layout;

    luaA_checktable(L, 1);

    name = luaA_name_init(L);
    mwfact = luaA_getopt_number(L, 1, "mwfact", 0.5);
    ncol = luaA_getopt_number(L, 1, "ncol", 1);
    nmaster = luaA_getopt_number(L, 1, "nmaster", 1);
    lay = luaA_getopt_string(L, 1, "layout", "tile");

    layout = name_func_lookup(lay, LayoutList);

    workspace = workspace_new(name,
                              layout,
                              mwfact, nmaster, ncol);

    return luaA_workspace_userdata_new(workspace);
}

/** Set a workspace visible on a screen.
 * \param A screen number.
 */
static int
luaA_workspace_screen_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    int screen = luaL_checknumber(L, 2) - 1;
    luaA_checkscreen(screen);
    workspace_view(*workspace, screen);
    return 0;
}

/** Get the screen the workspace is visible on.
 * \return A screen number of nil if the workspace is not visible.
 */
static int
luaA_workspace_screen_get(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    int screen = workspace_screen_get(*workspace);
    if(screen < 0)
        return 0;
    lua_pushnumber(L, screen + 1);
    return 1;
}

/** Give the focus to the latest focused client on a workspace.
 */
static int
luaA_workspace_focus_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    client_focus(focus_client_getcurrent(*workspace));
    return 0;
}

/** Set the workspace master width factor. This value is used in various layouts to
 * determine the size of the master window.
 * \param The master width ratio value, between 0 and 1.
 */
static int
luaA_workspace_mwfact_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    double mwfact = luaL_checknumber(L, 2);

    if(mwfact < 1 && mwfact > 0)
    {
        (*workspace)->mwfact = mwfact;
        (*workspace)->need_arrange = true;
    }
    else
        luaL_error(L, "bad value, must be between 0 and 1");

    return 0;
}

/** Get the workspace master width factor.
 * \return The master width ratio value.
 */
static int
luaA_workspace_mwfact_get(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    lua_pushnumber(L, (*workspace)->mwfact);
    return 1;
}

/** Set the number of columns. This is used in various layouts to set the number
 * of columns used to display non-master windows.
 * \param The number of columns, at least 1.
 */
static int
luaA_workspace_ncol_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    int ncol = luaL_checknumber(L, 2);

    if(ncol >= 1)
    {
        (*workspace)->ncol = ncol;
        (*workspace)->need_arrange = true;
    }
    else
        luaL_error(L, "bad value, must be greater than 1");

    return 0;
}

/** Get the number of columns used to display non-master windows on this workspace.
 * \return The number of column.
 */
static int
luaA_workspace_ncol_get(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    lua_pushnumber(L, (*workspace)->ncol);
    return 1;
}

/** Set the number of master windows. This is used in various layouts to
 * determine how many windows are in the master area.
 * \param The number of master windows.
 */
static int
luaA_workspace_nmaster_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    int nmaster = luaL_checknumber(L, 2);

    if(nmaster >= 0)
    {
        (*workspace)->nmaster = nmaster;
        (*workspace)->need_arrange = true;
    }
    else
        luaL_error(L, "bad value, must be greater than 0");

    return 0;
}

/** Get the number of master windows of the workspace.
 * \return The number of master windows.
 */
static int
luaA_workspace_nmaster_get(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    lua_pushnumber(L, (*workspace)->nmaster);
    return 1;
}

/** Get the workspace name.
 * \return The workspace name.
 */
static int
luaA_workspace_name_get(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    lua_pushstring(L, (*workspace)->name);
    return 1;
}

/** Set the workspace name.
 * \param A string with the new workspace name.
 */
static int
luaA_workspace_name_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    const char *name = luaL_checkstring(L, 2);
    p_delete(&(*workspace)->name);
    (*workspace)->name = a_strdup(name);
    return 0;
}

/** Handle workspace garbage collection.
 */
static int
luaA_workspace_gc(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    workspace_unref(workspace);
    *workspace = NULL;
    return 0;
}

/** Get the visible workspace for a screen.
 * \param A screen number.
 */
static int
luaA_workspace_visible_get(lua_State *L)
{
    int screen = luaL_checknumber(L, 1) - 1;
    luaA_checkscreen(screen);

    if(globalconf.screens[screen].workspace)
        return luaA_workspace_userdata_new(globalconf.screens[screen].workspace);
    return 0;
}

/** Get the layout of the workspace.
 * \return The layout name.
 */
static int
luaA_workspace_layout_get(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    const char *name = a_strdup(name_func_rlookup((*workspace)->layout, LayoutList));
    lua_pushstring(L, name);
    return 1;
}

/** Set the layout of the workspace.
 * \param A layout name.
 */
static int
luaA_workspace_layout_set(lua_State *L)
{
    workspace_t **workspace = luaA_checkudata(L, 1, "workspace");
    const char *name = luaL_checkstring(L, 2);
    layout_t *l = name_func_lookup(name, LayoutList);

    if(l)
    {
        (*workspace)->layout = l;
        (*workspace)->need_arrange = true;
    }
    else
        luaL_error(L, "unknown layout: %s", name);

    return 0;
}

/** Create a new userdata from a workspace.
 * \param t The workspace.
 * \return The luaA_settype return value.
 */
int
luaA_workspace_userdata_new(workspace_t *w)
{
    workspace_t **ws = lua_newuserdata(globalconf.L, sizeof(workspace_t *));
    *ws = w;
    workspace_ref(ws);
    return luaA_settype(globalconf.L, "workspace");
}

const struct luaL_reg awesome_workspace_methods[] =
{
    { "new", luaA_workspace_new },
    { "get", luaA_workspace_get},
    { "visible_get", luaA_workspace_visible_get },
    { NULL, NULL }
};
const struct luaL_reg awesome_workspace_meta[] =
{
    { "add", luaA_workspace_add },
    { "screen_set", luaA_workspace_screen_set },
    { "screen_get", luaA_workspace_screen_get },
    { "focus_set", luaA_workspace_focus_set },
    { "mwfact_set", luaA_workspace_mwfact_set },
    { "mwfact_get", luaA_workspace_mwfact_get },
    { "ncol_set", luaA_workspace_ncol_set },
    { "ncol_get", luaA_workspace_ncol_get },
    { "nmaster_set", luaA_workspace_nmaster_set },
    { "nmaster_get", luaA_workspace_nmaster_get },
    { "name_get", luaA_workspace_name_get },
    { "name_set", luaA_workspace_name_set },
    { "layout_get", luaA_workspace_layout_get },
    { "layout_set", luaA_workspace_layout_set },
    { "__eq", luaA_workspace_eq },
    { "__gc", luaA_workspace_gc },
    { "__tostring", luaA_workspace_tostring },
    { NULL, NULL },
};
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
