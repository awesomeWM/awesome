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

#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>

#include "screen.h"
#include "ewmh.h"
#include "objects/tag.h"
#include "objects/client.h"
#include "objects/drawin.h"
#include "luaa.h"
#include "common/xutil.h"

struct screen_output_t
{
    /** The XRandR names of the output */
    char *name;
    /** The size in millimeters */
    uint32_t mm_width, mm_height;
};

ARRAY_FUNCS(screen_output_t, screen_output, DO_NOTHING)

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
screen_add(screen_t new_screen)
{
    foreach(screen_to_test, globalconf.screens)
        if(new_screen.geometry.x == screen_to_test->geometry.x
           && new_screen.geometry.y == screen_to_test->geometry.y)
            {
                /* we already have a screen for this area, just check if
                 * it's not bigger and drop it */
                screen_to_test->geometry.width =
                    MAX(new_screen.geometry.width, screen_to_test->geometry.width);
                screen_to_test->geometry.height =
                    MAX(new_screen.geometry.height, screen_to_test->geometry.height);
                return;
            }

    signal_add(&new_screen.signals, "property::workarea");
    screen_array_append(&globalconf.screens, new_screen);

    /* Allocate the lua userdata object representing this screen */
    screen_t *s = &globalconf.screens.tab[globalconf.screens.len-1];
    screen_t **ps = lua_newuserdata(globalconf.L, sizeof(*ps));
    *ps = s;
    luaL_getmetatable(globalconf.L, "screen");
    lua_setmetatable(globalconf.L, -2);
    s->userdata = luaA_object_ref(globalconf.L, -1);
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
                /* Get info on the output crtc */
                xcb_randr_get_crtc_info_cookie_t crtc_info_c = xcb_randr_get_crtc_info(globalconf.connection, randr_crtcs[i], XCB_CURRENT_TIME);
                xcb_randr_get_crtc_info_reply_t *crtc_info_r = xcb_randr_get_crtc_info_reply(globalconf.connection, crtc_info_c, NULL);

                /* If CRTC has no OUTPUT, ignore it */
                if(!xcb_randr_get_crtc_info_outputs_length(crtc_info_r))
                    continue;

                /* Prepare the new screen */
                screen_t new_screen;
                p_clear(&new_screen, 1);
                new_screen.geometry.x = crtc_info_r->x;
                new_screen.geometry.y = crtc_info_r->y;
                new_screen.geometry.width= crtc_info_r->width;
                new_screen.geometry.height= crtc_info_r->height;

                xcb_randr_output_t *randr_outputs = xcb_randr_get_crtc_info_outputs(crtc_info_r);

                for(int j = 0; j < xcb_randr_get_crtc_info_outputs_length(crtc_info_r); j++)
                {
                    xcb_randr_get_output_info_cookie_t output_info_c = xcb_randr_get_output_info(globalconf.connection, randr_outputs[j], XCB_CURRENT_TIME);
                    xcb_randr_get_output_info_reply_t *output_info_r = xcb_randr_get_output_info_reply(globalconf.connection, output_info_c, NULL);

                    int len = xcb_randr_get_output_info_name_length(output_info_r);
                    /* name is not NULL terminated */
                    char *name = memcpy(p_new(char *, len + 1), xcb_randr_get_output_info_name(output_info_r), len);
                    name[len] = '\0';

                    screen_output_array_append(&new_screen.outputs,
                                               (screen_output_t) { .name = name,
                                                                   .mm_width = output_info_r->mm_width,
                                                                   .mm_height = output_info_r->mm_height });

                    p_delete(&output_info_r);
                }

                screen_add(new_screen);

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
            screen_t s;
            p_clear(&s, 1);
            s.geometry = screen_xsitoarea(xsi[screen]);
            screen_add(s);
        }

        p_delete(&xsq);

        return screens_exist();
    }

    return false;
}

static void screen_scan_x11(void)
{
    /* One screen only / Zaphod mode */
    xcb_screen_t *xcb_screen = globalconf.screen;
    screen_t s;
    p_clear(&s, 1);
    s.geometry.x = 0;
    s.geometry.y = 0;
    s.geometry.width = xcb_screen->width_in_pixels;
    s.geometry.height = xcb_screen->height_in_pixels;
    screen_add(s);
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
        if((x < 0 || (x >= s->geometry.x && x < s->geometry.x + s->geometry.width))
           && (y < 0 || (y >= s->geometry.y && y < s->geometry.y + s->geometry.height)))
            return s;

    /* No screen found, let's be creative. */
    return &globalconf.screens.tab[0];
}

/** Get screens info.
 * \param screen Screen.
 * \param strut Honor windows strut.
 * \return The screen area.
 */
area_t
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
    screen_t *old_screen = c->screen;
    area_t from, to;
    bool had_focus = false;

    if(new_screen == c->screen)
        return;

    if (globalconf.focus.client == c)
        had_focus = true;

    c->screen = new_screen;

    if(!doresize)
    {
        luaA_object_push(globalconf.L, c);
        luaA_object_emit_signal(globalconf.L, -1, "property::screen", 0);
        lua_pop(globalconf.L, 1);
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
    luaA_object_push(globalconf.L, c);
    luaA_object_emit_signal(globalconf.L, -1, "property::screen", 0);
    lua_pop(globalconf.L, 1);
    if(had_focus)
        client_focus(c);
}

/** Push a screen onto the stack.
 * \param L The Lua VM state.
 * \param s The screen to push.
 * \return The number of elements pushed on stack.
 */
static int
luaA_pushscreen(lua_State *L, screen_t *s)
{
    luaA_object_push(L, s->userdata);
    return 1;
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

    if(lua_type(L, 2) == LUA_TSTRING && (name = lua_tostring(L, 2)))
        foreach(screen, globalconf.screens)
            foreach(output, screen->outputs)
                if(A_STREQ(output->name, name))
                    return luaA_pushscreen(L, screen);

    int screen = luaL_checknumber(L, 2) - 1;
    luaA_checkscreen(screen);
    return luaA_pushscreen(L, &globalconf.screens.tab[screen]);
}

/** A screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield coords The screen coordinates. Immutable.
 * \lfield workarea The screen workarea.
 */
static int
luaA_screen_index(lua_State *L)
{
    const char *buf;
    screen_t **ps;
    screen_t *s;

    /* Get metatable of the screen. */
    lua_getmetatable(L, 1);
    /* Get the field */
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    /* Do we have a field like that? */
    if(!lua_isnil(L, -1))
    {
        /* Yes, so return it! */
        lua_remove(L, -2);
        return 1;
    }
    /* No, so remove everything. */
    lua_pop(L, 2);

    buf = luaL_checkstring(L, 2);
    ps = luaL_checkudata(L, 1, "screen");
    s = *ps;

    if(A_STREQ(buf, "index"))
    {
        lua_pushinteger(L, screen_array_indexof(&globalconf.screens, s) + 1);
        return 1;
    }

    if(A_STREQ(buf, "geometry"))
    {
        luaA_pusharea(L, s->geometry);
        return 1;
    }

    if(A_STREQ(buf, "workarea"))
    {
        luaA_pusharea(L, screen_area_get(s, true));
        return 1;
    }

    if(A_STREQ(buf, "outputs"))
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

    return luaA_default_index(L);
}

/** Add a signal to a screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A screen.
 * \lparam A signal name.
 */
static int
luaA_screen_add_signal(lua_State *L)
{
    screen_t **ps = luaL_checkudata(L, 1, "screen");
    screen_t *s = *ps;
    const char *name = luaL_checkstring(L, 2);
    signal_add(&s->signals, name);
    return 0;
}

/** Connect a screen's signal.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A screen.
 * \lparam A signal name.
 * \lparam A function to call when the signal is emitted.
 */
static int
luaA_screen_connect_signal(lua_State *L)
{
    screen_t **ps = luaL_checkudata(L, 1, "screen");
    screen_t *s = *ps;
    const char *name = luaL_checkstring(L, 2);
    luaA_checkfunction(L, 3);
    signal_connect(&s->signals, name, luaA_object_ref(L, 3));
    return 0;
}

/** Remove a signal to a screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A screen.
 * \lparam A signal name.
 * \lparam A function to remove
 */
static int
luaA_screen_disconnect_signal(lua_State *L)
{
    screen_t **ps = luaL_checkudata(L, 1, "screen");
    screen_t *s = *ps;
    const char *name = luaL_checkstring(L, 2);
    luaA_checkfunction(L, 3);
    const void *ref = lua_topointer(L, 3);
    signal_disconnect(&s->signals, name, ref);
    luaA_object_unref(L, (void *) ref);
    return 0;
}

/** Emit a signal to a screen.
 * \param L The Lua VM state.
 * \param screen The screen.
 * \param name The signal name.
 * \param nargs The number of arguments to the signal function.
 */
void
screen_emit_signal(lua_State *L, screen_t *screen, const char *name, int nargs)
{
    luaA_pushscreen(L, screen);
    lua_insert(L, - nargs - 1);
    signal_object_emit(L, &screen->signals, name, nargs + 1);
}

/** Emit a signal to a screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A screen.
 * \lparam A signal name.
 * \lparam Various arguments, optional.
 */
static int
luaA_screen_emit_signal(lua_State *L)
{
    screen_t **ps = luaL_checkudata(L, 1, "screen");
    screen_emit_signal(L, *ps, luaL_checkstring(L, 2), lua_gettop(L) - 2);
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
    lua_pushnumber(L, globalconf.screens.len);
    return 1;
}

const struct luaL_Reg awesome_screen_methods[] =
{
    { "count", luaA_screen_count },
    { "__index", luaA_screen_module_index },
    { "__newindex", luaA_default_newindex },
    { NULL, NULL }
};

const struct luaL_Reg awesome_screen_meta[] =
{
    { "add_signal", luaA_screen_add_signal },
    { "connect_signal", luaA_screen_connect_signal },
    { "disconnect_signal", luaA_screen_disconnect_signal },
    { "emit_signal", luaA_screen_emit_signal },
    { "__index", luaA_screen_index },
    { "__newindex", luaA_default_newindex },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
