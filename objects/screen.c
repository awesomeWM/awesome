/*
 * screen.c - screen management
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include "objects/screen.h"
#include "banning.h"
#include "objects/client.h"
#include "objects/drawin.h"

#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>

struct screen_output_t
{
    /** The XRandR names of the output */
    char *name;
    /** The size in millimeters */
    uint32_t mm_width, mm_height;
};

static void
screen_output_wipe(screen_output_t *output)
{
    p_delete(&output->name);
}

ARRAY_FUNCS(screen_output_t, screen_output, screen_output_wipe)

static lua_class_t screen_class;
LUA_OBJECT_FUNCS(screen_class, screen_t, screen)

/** Collect a screen. */
static void
screen_wipe(screen_t *s)
{
    screen_output_array_wipe(&s->outputs);
}

/** Get a screen argument from the lua stack */
screen_t *
luaA_checkscreen(lua_State *L, int sidx)
{
    if (lua_isnumber(L, sidx))
    {
        int screen = lua_tointeger(L, sidx);
        if(screen < 1 || screen > globalconf.screens.len)
            luaL_error(L, "invalid screen number: %d", screen);
        return globalconf.screens.tab[screen - 1];
    } else
        return luaA_checkudata(L, sidx, &screen_class);
}

static inline area_t
screen_xsitoarea(xcb_xinerama_screen_info_t si)
{
    area_t a =
    {
        .x = si.x_org,
        .y = si.y_org,
        .width = si.width,
        .height = si.height
    };
    return a;
}

static void
screen_add(lua_State *L, int sidx)
{
    screen_t *new_screen = luaA_checkudata(L, sidx, &screen_class);

    foreach(screen_to_test, globalconf.screens)
        if(new_screen->geometry.x == (*screen_to_test)->geometry.x
           && new_screen->geometry.y == (*screen_to_test)->geometry.y)
            {
                /* we already have a screen for this area, just check if
                 * it's not bigger and drop it */
                (*screen_to_test)->geometry.width =
                    MAX(new_screen->geometry.width, (*screen_to_test)->geometry.width);
                (*screen_to_test)->geometry.height =
                    MAX(new_screen->geometry.height, (*screen_to_test)->geometry.height);
                lua_remove(L, sidx);
                return;
            }

    luaA_object_ref(L, sidx);
    screen_array_append(&globalconf.screens, new_screen);
}

static bool
screens_exist(void)
{
    return globalconf.screens.len > 0;
}

static bool
screen_scan_randr(void)
{
    /* Check for extension before checking for XRandR */
    if(xcb_get_extension_data(globalconf.connection, &xcb_randr_id)->present)
    {
        xcb_randr_query_version_reply_t *version_reply =
            xcb_randr_query_version_reply(globalconf.connection,
                                          xcb_randr_query_version(globalconf.connection, 1, 1), 0);
        if(version_reply)
        {
            p_delete(&version_reply);

            /* A quick XRandR recall:
             * You have CRTC that manages a part of a SCREEN.
             * Each CRTC can draw stuff on one or more OUTPUT. */
            xcb_randr_get_screen_resources_cookie_t screen_res_c = xcb_randr_get_screen_resources(globalconf.connection, globalconf.screen->root);
            xcb_randr_get_screen_resources_reply_t *screen_res_r = xcb_randr_get_screen_resources_reply(globalconf.connection, screen_res_c, NULL);

            /* Only use the data from XRandR if there is more than one screen
             * defined. This should work around the broken nvidia driver.  */
            if (screen_res_r->num_crtcs <= 1)
            {
                p_delete(&screen_res_r);
                return false;
            }

            /* We go through CRTC, and build a screen for each one. */
            xcb_randr_crtc_t *randr_crtcs = xcb_randr_get_screen_resources_crtcs(screen_res_r);

            for(int i = 0; i < screen_res_r->num_crtcs; i++)
            {
                lua_State *L = globalconf_get_lua_State();

                /* Get info on the output crtc */
                xcb_randr_get_crtc_info_cookie_t crtc_info_c = xcb_randr_get_crtc_info(globalconf.connection, randr_crtcs[i], XCB_CURRENT_TIME);
                xcb_randr_get_crtc_info_reply_t *crtc_info_r = xcb_randr_get_crtc_info_reply(globalconf.connection, crtc_info_c, NULL);

                /* If CRTC has no OUTPUT, ignore it */
                if(!xcb_randr_get_crtc_info_outputs_length(crtc_info_r))
                    continue;

                /* Prepare the new screen */
                screen_t *new_screen = screen_new(L);
                new_screen->geometry.x = crtc_info_r->x;
                new_screen->geometry.y = crtc_info_r->y;
                new_screen->geometry.width= crtc_info_r->width;
                new_screen->geometry.height= crtc_info_r->height;

                xcb_randr_output_t *randr_outputs = xcb_randr_get_crtc_info_outputs(crtc_info_r);

                for(int j = 0; j < xcb_randr_get_crtc_info_outputs_length(crtc_info_r); j++)
                {
                    xcb_randr_get_output_info_cookie_t output_info_c = xcb_randr_get_output_info(globalconf.connection, randr_outputs[j], XCB_CURRENT_TIME);
                    xcb_randr_get_output_info_reply_t *output_info_r = xcb_randr_get_output_info_reply(globalconf.connection, output_info_c, NULL);

                    int len = xcb_randr_get_output_info_name_length(output_info_r);
                    /* name is not NULL terminated */
                    char *name = memcpy(p_new(char *, len + 1), xcb_randr_get_output_info_name(output_info_r), len);
                    name[len] = '\0';

                    screen_output_array_append(&new_screen->outputs,
                                               (screen_output_t) { .name = name,
                                                                   .mm_width = output_info_r->mm_width,
                                                                   .mm_height = output_info_r->mm_height });

                    p_delete(&output_info_r);
                }

                screen_add(L, -1);

                p_delete(&crtc_info_r);
            }

            p_delete(&screen_res_r);

            return screens_exist();
        }
    }

    return false;
}

static bool
screen_scan_xinerama(void)
{
    bool xinerama_is_active = false;

    /* Check for extension before checking for Xinerama */
    if(xcb_get_extension_data(globalconf.connection, &xcb_xinerama_id)->present)
    {
        xcb_xinerama_is_active_reply_t *xia;
        xia = xcb_xinerama_is_active_reply(globalconf.connection, xcb_xinerama_is_active(globalconf.connection), NULL);
        xinerama_is_active = xia->state;
        p_delete(&xia);
    }

    if(xinerama_is_active)
    {
        xcb_xinerama_query_screens_reply_t *xsq;
        xcb_xinerama_screen_info_t *xsi;
        int xinerama_screen_number;

        xsq = xcb_xinerama_query_screens_reply(globalconf.connection,
                                               xcb_xinerama_query_screens_unchecked(globalconf.connection),
                                               NULL);

        xsi = xcb_xinerama_query_screens_screen_info(xsq);
        xinerama_screen_number = xcb_xinerama_query_screens_screen_info_length(xsq);

        /* now check if screens overlaps (same x,y): if so, we take only the biggest one */
        for(int screen = 0; screen < xinerama_screen_number; screen++)
        {
            lua_State *L = globalconf_get_lua_State();
            screen_t *s = screen_new(L);
            s->geometry = screen_xsitoarea(xsi[screen]);
            screen_add(L, -1);
        }

        p_delete(&xsq);

        return screens_exist();
    }

    return false;
}

static void screen_scan_x11(void)
{
    /* One screen only / Zaphod mode */
    lua_State *L = globalconf_get_lua_State();
    xcb_screen_t *xcb_screen = globalconf.screen;
    screen_t *s = screen_new(L);
    s->geometry.x = 0;
    s->geometry.y = 0;
    s->geometry.width = xcb_screen->width_in_pixels;
    s->geometry.height = xcb_screen->height_in_pixels;
    screen_add(L, -1);
}

/** Get screens informations and fill global configuration.
 */
void
screen_scan(void)
{
    if(!screen_scan_randr() && !screen_scan_xinerama())
        screen_scan_x11();
}

/** Return the Xinerama screen number where the coordinates belongs to.
 * \param screen The logical screen number.
 * \param x X coordinate
 * \param y Y coordinate
 * \return Screen pointer or screen param if no match or no multi-head.
 */
screen_t *
screen_getbycoord(int x, int y)
{
    foreach(s, globalconf.screens)
        if((x < 0 || (x >= (*s)->geometry.x && x < (*s)->geometry.x + (*s)->geometry.width))
           && (y < 0 || (y >= (*s)->geometry.y && y < (*s)->geometry.y + (*s)->geometry.height)))
            return *s;

    /* No screen found, let's be creative. */
    return globalconf.screens.tab[0];
}

/** Get screens info.
 * \param screen Screen.
 * \param strut Honor windows strut.
 * \return The screen area.
 */
static area_t
screen_area_get(screen_t *screen, bool strut)
{
    if(!strut)
        return screen->geometry;

    area_t area = screen->geometry;
    uint16_t top = 0, bottom = 0, left = 0, right = 0;

#define COMPUTE_STRUT(o) \
    { \
        if((o)->strut.top_start_x || (o)->strut.top_end_x || (o)->strut.top) \
        { \
            if((o)->strut.top) \
                top = MAX(top, (o)->strut.top); \
            else \
                top = MAX(top, ((o)->geometry.y - area.y) + (o)->geometry.height); \
        } \
        if((o)->strut.bottom_start_x || (o)->strut.bottom_end_x || (o)->strut.bottom) \
        { \
            if((o)->strut.bottom) \
                bottom = MAX(bottom, (o)->strut.bottom); \
            else \
                bottom = MAX(bottom, (area.y + area.height) - (o)->geometry.y); \
        } \
        if((o)->strut.left_start_y || (o)->strut.left_end_y || (o)->strut.left) \
        { \
            if((o)->strut.left) \
                left = MAX(left, (o)->strut.left); \
            else \
                left = MAX(left, ((o)->geometry.x - area.x) + (o)->geometry.width); \
        } \
        if((o)->strut.right_start_y || (o)->strut.right_end_y || (o)->strut.right) \
        { \
            if((o)->strut.right) \
                right = MAX(right, (o)->strut.right); \
            else \
                right = MAX(right, (area.x + area.width) - (o)->geometry.x); \
        } \
    }

    foreach(c, globalconf.clients)
        if((*c)->screen == screen && client_isvisible(*c))
            COMPUTE_STRUT(*c)

    foreach(drawin, globalconf.drawins)
        if((*drawin)->visible)
        {
            screen_t *d_screen =
                screen_getbycoord((*drawin)->geometry.x, (*drawin)->geometry.y);
            if (d_screen == screen)
                COMPUTE_STRUT(*drawin)
        }

#undef COMPUTE_STRUT

    area.x += left;
    area.y += top;
    area.width -= left + right;
    area.height -= top + bottom;

    return area;
}

/** Get display info.
 * \return The display area.
 */
area_t
display_area_get(void)
{
    xcb_screen_t *s = globalconf.screen;
    area_t area = { .x = 0,
                    .y = 0,
                    .width = s->width_in_pixels,
                    .height = s->height_in_pixels };
    return area;
}

/** Move a client to a virtual screen.
 * \param c The client to move.
 * \param new_screen The destination screen.
 * \param doresize Set to true if we also move the client to the new x and
 *        y of the new screen.
 */
void
screen_client_moveto(client_t *c, screen_t *new_screen, bool doresize)
{
    lua_State *L = globalconf_get_lua_State();
    screen_t *old_screen = c->screen;
    int old_screen_idx = screen_get_index(old_screen);
    area_t from, to;
    bool had_focus = false;

    if(new_screen == c->screen)
        return;

    if (globalconf.focus.client == c)
        had_focus = true;

    c->screen = new_screen;

    if(!doresize)
    {
        luaA_object_push(L, c);
        if(old_screen_idx != 0)
            lua_pushinteger(L, old_screen_idx);
        else
            lua_pushnil(L);
        luaA_object_emit_signal(L, -2, "property::screen", 1);
        lua_pop(L, 1);
        if(had_focus)
            client_focus(c);
        return;
    }

    from = screen_area_get(old_screen, false);
    to = screen_area_get(c->screen, false);

    area_t new_geometry = c->geometry;

    new_geometry.x = to.x + new_geometry.x - from.x;
    new_geometry.y = to.y + new_geometry.y - from.y;

    /* resize the client if it doesn't fit the new screen */
    if(new_geometry.width > to.width)
        new_geometry.width = to.width;
    if(new_geometry.height > to.height)
        new_geometry.height = to.height;

    /* make sure the client is still on the screen */
    if(new_geometry.x + new_geometry.width > to.x + to.width)
        new_geometry.x = to.x + to.width - new_geometry.width;
    if(new_geometry.y + new_geometry.height > to.y + to.height)
        new_geometry.y = to.y + to.height - new_geometry.height;

    /* move / resize the client */
    client_resize(c, new_geometry, false);
    luaA_object_push(L, c);
    if(old_screen_idx != 0)
        lua_pushinteger(L, old_screen_idx);
    else
        lua_pushnil(L);
    luaA_object_emit_signal(L, -2, "property::screen", 1);
    lua_pop(L, 1);
    if(had_focus)
        client_focus(c);
}

/** Get a screen's index. */
int
screen_get_index(screen_t *s)
{
    int res = 0;
    foreach(screen, globalconf.screens)
    {
        res++;
        if (*screen == s)
            return res;
    }

    return 0;
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
    const char *name;

    if((name = lua_tostring(L, 2)))
        foreach(screen, globalconf.screens)
            foreach(output, (*screen)->outputs)
                if(A_STREQ(output->name, name))
                    return luaA_object_push(L, screen);

    return luaA_object_push(L, luaA_checkscreen(L, 2));
}

LUA_OBJECT_EXPORT_PROPERTY(screen, screen_t, geometry, luaA_pusharea)

static int
luaA_screen_get_index(lua_State *L, screen_t *s)
{
    lua_pushinteger(L, screen_get_index(s));
    return 1;
}

static int
luaA_screen_get_outputs(lua_State *L, screen_t *s)
{
    lua_createtable(L, 0, s->outputs.len);
    foreach(output, s->outputs)
    {
        lua_createtable(L, 2, 0);

        lua_pushinteger(L, output->mm_width);
        lua_setfield(L, -2, "mm_width");
        lua_pushinteger(L, output->mm_height);
        lua_setfield(L, -2, "mm_height");

        lua_setfield(L, -2, output->name);
    }
    /* The table of tables we created. */
    return 1;
}

static int
luaA_screen_get_workarea(lua_State *L, screen_t *s)
{
    luaA_pusharea(L, screen_area_get(s, true));
    return 1;
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
    lua_pushnumber(L, globalconf.screens.len);
    return 1;
}

void
screen_class_setup(lua_State *L)
{
    static const struct luaL_Reg screen_methods[] =
    {
        LUA_CLASS_METHODS(screen)
        { "count", luaA_screen_count },
        { "__index", luaA_screen_module_index },
        { "__newindex", luaA_default_newindex },
        { NULL, NULL }
    };

    static const struct luaL_Reg screen_meta[] =
    {
        LUA_OBJECT_META(screen)
        LUA_CLASS_META
        { NULL, NULL },
    };

    luaA_class_setup(L, &screen_class, "screen", NULL,
                     (lua_class_allocator_t) screen_new,
                     (lua_class_collector_t) screen_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     screen_methods, screen_meta);
    luaA_class_add_property(&screen_class, "geometry",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_geometry,
                            NULL);
    luaA_class_add_property(&screen_class, "index",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_index,
                            NULL);
    luaA_class_add_property(&screen_class, "outputs",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_outputs,
                            NULL);
    luaA_class_add_property(&screen_class, "workarea",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_workarea,
                            NULL);
    signal_add(&screen_class.signals, "property::workarea");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
