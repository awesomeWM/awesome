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

#include "tag.h"
#include "client.h"
#include "ewmh.h"
#include "luaa.h"

/** Tag type */
struct tag
{
    LUA_OBJECT_HEADER
    /** Tag name */
    char *name;
    /** true if activated */
    bool activated;
    /** true if selected */
    bool selected;
    /** clients in this tag */
    client_array_t clients;
};

static lua_class_t tag_class;
LUA_OBJECT_FUNCS(tag_class, tag_t, tag)


void
tag_unref_simplified(tag_t **tag)
{
    luaA_object_unref(globalconf.L, *tag);
}

static void
tag_wipe(tag_t *tag)
{
    client_array_wipe(&tag->clients);
    p_delete(&tag->name);
}

OBJECT_EXPORT_PROPERTY(tag, tag_t, selected)
OBJECT_EXPORT_PROPERTY(tag, tag_t, name)

/** View or unview a tag.
 * \param L The Lua VM state.
 * \param udx The index of the tag on the stack.
 * \param view Set selected or not.
 */
static void
tag_view(lua_State *L, int udx, bool view)
{
    tag_t *tag = luaA_checkudata(L, udx, &tag_class);
    if(tag->selected != view)
    {
        tag->selected = view;
        banning_need_update();
        ewmh_update_net_current_desktop();

        luaA_object_emit_signal(L, udx, "property::selected", 0);
    }
}

static void
tag_client_emit_signal(lua_State *L, tag_t *t, client_t *c, const char *signame)
{
    luaA_object_push(L, c);
    luaA_object_push(L, t);
    /* emit signal on client, with new tag as argument */
    luaA_object_emit_signal(L, -2, signame, 1);
    /* re push tag */
    luaA_object_push(L, t);
    /* move tag before client */
    lua_insert(L, -2);
    luaA_object_emit_signal(L, -2, signame, 1);
    /* Remove tag */
    lua_pop(L, 1);
}

/** Tag a client with the tag on top of the stack.
 * \param c the client to tag
 */
void
tag_client(client_t *c)
{
    tag_t *t = luaA_object_ref_class(globalconf.L, -1, &tag_class);

    /* don't tag twice */
    if(is_client_tagged(c, t))
    {
        luaA_object_unref(globalconf.L, t);
        return;
    }

    client_array_append(&t->clients, c);
    ewmh_client_update_desktop(c);
    banning_need_update();

    tag_client_emit_signal(globalconf.L, t, c, "tagged");
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
            banning_need_update();
            ewmh_client_update_desktop(c);
            tag_client_emit_signal(globalconf.L, t, c, "untagged");
            luaA_object_unref(globalconf.L, t);
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

/** Get the index of the first selected tag.
 * \return Its index.
 */
int
tags_get_first_selected_index(void)
{
    foreach(tag, globalconf.tags)
        if((*tag)->selected)
            return tag_array_indexof(&globalconf.tags, tag);
    return 0;
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
    return luaA_class_new(L, &tag_class);
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
    tag_t *tag = luaA_checkudata(L, 1, &tag_class);
    client_array_t *clients = &tag->clients;
    int i;

    if(lua_gettop(L) == 2)
    {
        luaA_checktable(L, 2);
        foreach(c, tag->clients)
        {
            /* Only untag if we aren't going to add this tag again */
            bool found = false;
            lua_pushnil(L);
            while(lua_next(L, 2))
            {
                client_t *tc = luaA_checkudata(L, -1, &client_class);
                /* Pop the value from lua_next */
                lua_pop(L, 1);
                if (tc != *c)
                    continue;

                /* Pop the key from lua_next */
                lua_pop(L, 1);
                found = true;
                break;
            }
            if(!found)
                untag_client(*c, tag);
        }
        lua_pushnil(L);
        while(lua_next(L, 2))
        {
            client_t *c = luaA_checkudata(L, -1, &client_class);
            /* push tag on top of the stack */
            lua_pushvalue(L, 1);
            tag_client(c);
            lua_pop(L, 1);
        }
    }

    lua_createtable(L, clients->len, 0);
    for(i = 0; i < clients->len; i++)
    {
        luaA_object_push(L, clients->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

LUA_OBJECT_EXPORT_PROPERTY(tag, tag_t, name, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(tag, tag_t, selected, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(tag, tag_t, activated, lua_pushboolean)

/** Set the tag name.
 * \param L The Lua VM state.
 * \param tag The tag to name.
 * \return The number of elements pushed on stack.
 */
static int
luaA_tag_set_name(lua_State *L, tag_t *tag)
{
    size_t len;
    const char *buf = luaL_checklstring(L, -1, &len);
    p_delete(&tag->name);
    a_iso2utf8(buf, len, &tag->name, NULL);
    luaA_object_emit_signal(L, -3, "property::name", 0);
    ewmh_update_net_desktop_names();
    return 0;
}

/** Set the tag selection status.
 * \param L The Lua VM state.
 * \param tag The tag to set the selection status for.
 * \return The number of elements pushed on stack.
 */
static int
luaA_tag_set_selected(lua_State *L, tag_t *tag)
{
    tag_view(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

/** Set the tag activated status.
 * \param L The Lua VM state.
 * \param tag The tag to set the activated status for.
 * \return The number of elements pushed on stack.
 */
static int
luaA_tag_set_activated(lua_State *L, tag_t *tag)
{
    bool activated = luaA_checkboolean(L, -1);
    if(activated == tag->activated)
        return 0;

    tag->activated = activated;
    if(activated)
    {
        lua_pushvalue(L, -3);
        tag_array_append(&globalconf.tags, luaA_object_ref_class(L, -1, &tag_class));
    }
    else
    {
        for (int i = 0; i < globalconf.tags.len; i++)
            if(globalconf.tags.tab[i] == tag)
            {
                tag_array_take(&globalconf.tags, i);
                break;
            }

        if (tag->selected)
        {
            tag->selected = false;
            luaA_object_emit_signal(L, -3, "property::selected", 0);
            banning_need_update();
        }
        luaA_object_unref(L, tag);
    }
    ewmh_update_net_numbers_of_desktop();
    ewmh_update_net_desktop_names();

    luaA_object_emit_signal(L, -3, "property::activated", 0);

    return 0;
}

void
tag_class_setup(lua_State *L)
{
    static const struct luaL_Reg tag_methods[] =
    {
        LUA_CLASS_METHODS(tag)
        { "__call", luaA_tag_new },
        { NULL, NULL }
    };

    static const struct luaL_Reg tag_meta[] =
    {
        LUA_OBJECT_META(tag)
        LUA_CLASS_META
        { "clients", luaA_tag_clients },
        { NULL, NULL },
    };

    luaA_class_setup(L, &tag_class, "tag", NULL,
                     (lua_class_allocator_t) tag_new,
                     (lua_class_collector_t) tag_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     tag_methods, tag_meta);
    luaA_class_add_property(&tag_class, "name",
                            (lua_class_propfunc_t) luaA_tag_set_name,
                            (lua_class_propfunc_t) luaA_tag_get_name,
                            (lua_class_propfunc_t) luaA_tag_set_name);
    luaA_class_add_property(&tag_class, "selected",
                            (lua_class_propfunc_t) luaA_tag_set_selected,
                            (lua_class_propfunc_t) luaA_tag_get_selected,
                            (lua_class_propfunc_t) luaA_tag_set_selected);
    luaA_class_add_property(&tag_class, "activated",
                            (lua_class_propfunc_t) luaA_tag_set_activated,
                            (lua_class_propfunc_t) luaA_tag_get_activated,
                            (lua_class_propfunc_t) luaA_tag_set_activated);

    signal_add(&tag_class.signals, "property::name");
    signal_add(&tag_class.signals, "property::selected");
    signal_add(&tag_class.signals, "property::activated");
    signal_add(&tag_class.signals, "request::select");
    signal_add(&tag_class.signals, "tagged");
    signal_add(&tag_class.signals, "untagged");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
