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

/** awesome screen API
 *
 * Screen objects can be added and removed over time. To get a callback for all
 * current and future screens, use `awful.screen.connect_for_each_screen`:
 *
 *    awful.screen.connect_for_each_screen(function(s)
 *        -- do something
 *    end)
 *
 * It is also possible loop over all current screens using:
 *
 *    for s, screen do
 *        -- do something
 *    end
 *
 * Most basic Awesome objects also have a screen property, see `mouse.screen`
 * `client.screen`, `wibox.screen` and `tag.screen`.
 *
 * Furthermore to the classes described here, one can also use signals as
 * described in @{signals}.
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @release @AWESOME_VERSION@
 * @classmod screen
 */

#include "objects/screen.h"
#include "banning.h"
#include "objects/client.h"
#include "objects/drawin.h"

#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>

/** Screen is a table where indexes are screen numbers. You can use `screen[1]`
 * to get access to the first screen, etc. Alternatively, if RANDR information
 * is available, you can use output names for finding screen objects.
 * The primary screen can be accessed as `screen.primary`.
 * Each screen has a set of properties.
 *
 */

 /**
  * The primary screen.
  *
  * @tfield screen primary
  */

/**
 * The screen coordinates.
 *
 * **Immutable:** true
 * @property geometry
 * @param table
 * @tfield integer table.x The horizontal position
 * @tfield integer table.y The vertical position
 * @tfield integer table.width The width
 * @tfield integer table.height The height
 */

/**
 * The screen number.
 *
 * An integer greater than 1 and smaller than `screen.count()`. Please note that
 * the screen order isn't always mirroring the screen logical position.
 *
 * **Immutable:** true
 * @property index
 * @param integer
 */

/**
 * If RANDR information is available, a list of outputs
 *   for this screen and their size in mm.
 *
 * Please note that the table content may vary.
 *
 * **Signal:**
 *
 *  * *property::outputs*
 *
 * **Immutable:** true
 * @property outputs
 * @param table
 * @tfield table table.name A table with the screen name as key (like `eDP1` on a laptop)
 * @tfield integer table.name.mm_width The screen physical width
 * @tfield integer table.name.mm_height The screen physical height
 */

/**
 * The screen workarea.
 *
 * The workarea is a subsection of the screen where clients can be placed. It
 * usually excludes the toolbars (see `awful.wibox`) and dockable clients
 * (see `client.dockable`) like WindowMaker DockAPP.
 *
 * It can be modified be altering the `wibox` or `client` struts.
 *
 * **Signal:**
 *
 *  * *property::workarea*
 *
 * @property workarea
 * @see client.struts
 * @see drawin.struts
 * @param table
 * @tfield integer table.x The horizontal position
 * @tfield integer table.y The vertical position
 * @tfield integer table.width The width
 * @tfield integer table.height The height
 */


/** Get the number of instances.
 *
 * @return The number of screen objects alive.
 * @function instances
 */

/* Set a __index metamethod for all screen instances.
 * @tparam function cb The meta-method
 * @function set_index_miss_handler
 */

/* Set a __newindex metamethod for all screen instances.
 * @tparam function cb The meta-method
 * @function set_newindex_miss_handler
 */

DO_ARRAY(xcb_randr_output_t, randr_output, DO_NOTHING);

struct screen_output_t
{
    /** The XRandR names of the output */
    char *name;
    /** The size in millimeters */
    uint32_t mm_width, mm_height;
    /** The XID */
    randr_output_array_t outputs;
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
            luaL_error(L, "invalid screen number: %d (of %d existing)",
                    screen, globalconf.screens.len);
        return globalconf.screens.tab[screen - 1];
    } else
        return luaA_checkudata(L, sidx, &screen_class);
}

static screen_t *
screen_add(lua_State *L, screen_array_t *screens)
{
    screen_t *new_screen = screen_new(L);
    luaA_object_ref(L, -1);
    screen_array_append(screens, new_screen);
    return new_screen;
}

/* Monitors were introduced in RandR 1.5 */
#ifdef XCB_RANDR_GET_MONITORS
static void
screen_scan_randr_monitors(lua_State *L, screen_array_t *screens)
{
    xcb_randr_get_monitors_cookie_t monitors_c = xcb_randr_get_monitors(globalconf.connection, globalconf.screen->root, 1);
    xcb_randr_get_monitors_reply_t *monitors_r = xcb_randr_get_monitors_reply(globalconf.connection, monitors_c, NULL);
    xcb_randr_monitor_info_iterator_t monitor_iter;
    bool found = false;

    for(monitor_iter = xcb_randr_get_monitors_monitors_iterator(monitors_r);
            monitor_iter.rem; xcb_randr_monitor_info_next(&monitor_iter))
    {
        screen_t *new_screen;
        screen_output_t output;
        xcb_randr_output_t *randr_outputs;
        xcb_get_atom_name_cookie_t name_c;
        xcb_get_atom_name_reply_t *name_r;

        if(!xcb_randr_monitor_info_outputs_length(monitor_iter.data))
            continue;

        new_screen = screen_add(L, screens);
        new_screen->geometry.x = monitor_iter.data->x;
        new_screen->geometry.y = monitor_iter.data->y;
        new_screen->geometry.width= monitor_iter.data->width;
        new_screen->geometry.height= monitor_iter.data->height;

        output.mm_width = monitor_iter.data->width_in_millimeters;
        output.mm_height = monitor_iter.data->height_in_millimeters;

        name_c = xcb_get_atom_name_unchecked(globalconf.connection, monitor_iter.data->name);
        name_r = xcb_get_atom_name_reply(globalconf.connection, name_c, NULL);
        if (name_r) {
            const char *name = xcb_get_atom_name_name(name_r);
            size_t len = xcb_get_atom_name_name_length(name_r);

            output.name = memcpy(p_new(char *, len + 1), name, len);
            output.name[len] = '\0';
            p_delete(&name_r);
        } else {
            output.name = a_strdup("unknown");
        }
        randr_output_array_init(&output.outputs);

        randr_outputs = xcb_randr_monitor_info_outputs(monitor_iter.data);
        for(int i = 0; i < xcb_randr_monitor_info_outputs_length(monitor_iter.data); i++) {
            randr_output_array_append(&output.outputs, randr_outputs[i]);
        }

        screen_output_array_append(&new_screen->outputs, output);
    }

    p_delete(&monitors_r);
}
#else
static void
screen_scan_randr_monitors(lua_State *L, screen_array_t *screens)
{
}
#endif

static void
screen_scan_randr_crtcs(lua_State *L, screen_array_t *screens)
{
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
        return;
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
        screen_t *new_screen = screen_add(L, screens);
        new_screen->geometry.x = crtc_info_r->x;
        new_screen->geometry.y = crtc_info_r->y;
        new_screen->geometry.width= crtc_info_r->width;
        new_screen->geometry.height= crtc_info_r->height;

        xcb_randr_output_t *randr_outputs = xcb_randr_get_crtc_info_outputs(crtc_info_r);

        for(int j = 0; j < xcb_randr_get_crtc_info_outputs_length(crtc_info_r); j++)
        {
            xcb_randr_get_output_info_cookie_t output_info_c = xcb_randr_get_output_info(globalconf.connection, randr_outputs[j], XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_r = xcb_randr_get_output_info_reply(globalconf.connection, output_info_c, NULL);
            screen_output_t output;

            int len = xcb_randr_get_output_info_name_length(output_info_r);
            /* name is not NULL terminated */
            char *name = memcpy(p_new(char *, len + 1), xcb_randr_get_output_info_name(output_info_r), len);
            name[len] = '\0';

            output.name = name;
            output.mm_width = output_info_r->mm_width;
            output.mm_height = output_info_r->mm_height;
            randr_output_array_init(&output.outputs);
            randr_output_array_append(&output.outputs, randr_outputs[j]);

            screen_output_array_append(&new_screen->outputs, output);


            p_delete(&output_info_r);
        }

        p_delete(&crtc_info_r);
    }

    p_delete(&screen_res_r);
}

static void
screen_scan_randr(lua_State *L, screen_array_t *screens)
{
    xcb_randr_query_version_reply_t *version_reply;
    uint32_t major_version;
    uint32_t minor_version;

    /* Check for extension before checking for XRandR */
    if(!xcb_get_extension_data(globalconf.connection, &xcb_randr_id)->present)
        return;

    version_reply =
        xcb_randr_query_version_reply(globalconf.connection,
                                      xcb_randr_query_version(globalconf.connection, 1, 5), 0);
    if(!version_reply)
        return;

    major_version = version_reply->major_version;
    minor_version = version_reply->minor_version;
    p_delete(&version_reply);

    /* Do we agree on a supported version? */
    if (major_version != 1 || minor_version < 2)
        return;

    globalconf.have_randr_13 = minor_version >= 3;

    if (minor_version >= 5)
        screen_scan_randr_monitors(L, screens);
    else
        screen_scan_randr_crtcs(L, screens);
}

static void
screen_scan_xinerama(lua_State *L, screen_array_t *screens)
{
    bool xinerama_is_active;
    xcb_xinerama_is_active_reply_t *xia;
    xcb_xinerama_query_screens_reply_t *xsq;
    xcb_xinerama_screen_info_t *xsi;
    int xinerama_screen_number;

    /* Check for extension before checking for Xinerama */
    if(!xcb_get_extension_data(globalconf.connection, &xcb_xinerama_id)->present)
        return;

    xia = xcb_xinerama_is_active_reply(globalconf.connection, xcb_xinerama_is_active(globalconf.connection), NULL);
    xinerama_is_active = xia->state;
    p_delete(&xia);
    if(!xinerama_is_active)
        return;

    xsq = xcb_xinerama_query_screens_reply(globalconf.connection,
                                           xcb_xinerama_query_screens_unchecked(globalconf.connection),
                                           NULL);

    xsi = xcb_xinerama_query_screens_screen_info(xsq);
    xinerama_screen_number = xcb_xinerama_query_screens_screen_info_length(xsq);

    for(int screen = 0; screen < xinerama_screen_number; screen++)
    {
        screen_t *s = screen_add(L, screens);
        s->geometry.x = xsi[screen].x_org;
        s->geometry.y = xsi[screen].y_org;
        s->geometry.width = xsi[screen].width;
        s->geometry.height = xsi[screen].height;
    }

    p_delete(&xsq);
}

static void screen_scan_x11(lua_State *L, screen_array_t *screens)
{
    xcb_screen_t *xcb_screen = globalconf.screen;
    screen_t *s = screen_add(L, screens);
    s->geometry.x = 0;
    s->geometry.y = 0;
    s->geometry.width = xcb_screen->width_in_pixels;
    s->geometry.height = xcb_screen->height_in_pixels;
}

/** Get screens informations and fill global configuration.
 */
void
screen_scan(void)
{
    lua_State *L;

    L = globalconf_get_lua_State();

    screen_scan_randr(L, &globalconf.screens);
    if (globalconf.screens.len == 0)
        screen_scan_xinerama(L, &globalconf.screens);
    if (globalconf.screens.len == 0)
        screen_scan_x11(L, &globalconf.screens);
    assert(globalconf.screens.len > 0);

    foreach(screen, globalconf.screens) {
        luaA_object_push(L, *screen);
        luaA_object_emit_signal(L, -1, "added", 0);
        lua_pop(L, 1);
    }

    screen_update_primary();
}

/** Return the squared distance of the given screen to the coordinates.
 * \param screen The screen
 * \param x X coordinate
 * \param y Y coordinate
 * \return Squared distance of the point to the screen.
 */
static unsigned int
screen_get_distance_squared(screen_t *s, int x, int y)
{
    int sx = s->geometry.x, sy = s->geometry.y;
    int sheight = s->geometry.height, swidth = s->geometry.width;
    unsigned int dist_x, dist_y;

    /* Calculate distance in X coordinate */
    if (x < sx)
        dist_x = sx - x;
    else if (x < sx + swidth)
        dist_x = 0;
    else
        dist_x = x - sx - swidth;

    /* Calculate distance in Y coordinate */
    if (y < sy)
        dist_y = sy - y;
    else if (y < sy + sheight)
        dist_y = 0;
    else
        dist_y = y - sy - sheight;

    return dist_x * dist_x + dist_y * dist_y;
}

/** Return the first screen number where the coordinates belong to.
 * \param x X coordinate
 * \param y Y coordinate
 * \return Screen pointer or screen param if no match or no multi-head.
 */
screen_t *
screen_getbycoord(int x, int y)
{
    foreach(s, globalconf.screens)
        if(screen_coord_in_screen(*s, x, y))
            return *s;

    /* No screen found, find nearest screen. */
    screen_t *nearest_screen = NULL;
    unsigned int nearest_dist = UINT_MAX;
    foreach(s, globalconf.screens)
    {
        unsigned int dist_sq = screen_get_distance_squared(*s, x, y);
        if(dist_sq < nearest_dist)
        {
            nearest_dist = dist_sq;
            nearest_screen = *s;
        }
    }
    return nearest_screen;
}

/** Are the given coordinates in a given screen?
 * \param screen The logical screen number.
 * \param x X coordinate
 * \param y Y coordinate
 * \return True if the X/Y coordinates are in the given screen.
 */
bool
screen_coord_in_screen(screen_t *s, int x, int y)
{
    return (x >= s->geometry.x && x < s->geometry.x + s->geometry.width)
           && (y >= s->geometry.y && y < s->geometry.y + s->geometry.height);
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
        if(old_screen != NULL)
            luaA_object_push(L, old_screen);
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

    /* Emit signal, but only in case the call to client_resize had not changed
     * it already. */
    if(old_screen != c->screen)
    {
        luaA_object_push(L, c);
        if(old_screen != NULL)
            luaA_object_push(L, old_screen);
        else
            lua_pushnil(L);
        luaA_object_emit_signal(L, -2, "property::screen", 1);
        lua_pop(L, 1);
    }

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

void
screen_update_primary(void)
{
    if (!globalconf.have_randr_13)
        return;

    screen_t *primary_screen = NULL;
    xcb_randr_get_output_primary_reply_t *primary =
        xcb_randr_get_output_primary_reply(globalconf.connection,
                xcb_randr_get_output_primary(globalconf.connection, globalconf.screen->root),
                NULL);

    if (!primary)
        return;

    foreach(screen, globalconf.screens)
    {
        foreach(output, (*screen)->outputs)
            foreach (randr_output, output->outputs)
                if (*randr_output == primary->output)
                    primary_screen = *screen;
    }
    p_delete(&primary);

    if (!primary_screen || primary_screen == globalconf.primary_screen)
        return;

    lua_State *L = globalconf_get_lua_State();
    screen_t *old = globalconf.primary_screen;
    globalconf.primary_screen = primary_screen;

    if (old)
    {
        luaA_object_push(L, old);
        luaA_object_emit_signal(L, -1, "primary_changed", 0);
        lua_pop(L, 1);
    }
    luaA_object_push(L, primary_screen);
    luaA_object_emit_signal(L, -1, "primary_changed", 0);
    lua_pop(L, 1);
}

screen_t *
screen_get_primary(void)
{
    if (!globalconf.primary_screen && globalconf.screens.len > 0)
        globalconf.primary_screen = globalconf.screens.tab[0];
    return globalconf.primary_screen;
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
    {
        if(A_STREQ(name, "primary"))
            return luaA_object_push(L, screen_get_primary());

        foreach(screen, globalconf.screens)
            foreach(output, (*screen)->outputs)
                if(A_STREQ(output->name, name))
                    return luaA_object_push(L, *screen);
    }

    return luaA_object_push(L, luaA_checkscreen(L, 2));
}

/** Iterate over screens.
 * @usage
 * for s in screen do
 *     print("Oh, wow, we have screen " .. tostring(s))
 * end
 * @function screen
 */
static int
luaA_screen_module_call(lua_State *L)
{
    int idx;

    if (lua_isnoneornil(L, 3))
        idx = 0;
    else
        idx = screen_get_index(luaA_checkscreen(L, 3));
    if (idx >= 0 && idx < globalconf.screens.len)
        /* No +1 needed, index starts at 1, C array at 0 */
        luaA_object_push(L, globalconf.screens.tab[idx]);
    else
        lua_pushnil(L);
    return 1;
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

/** Get the number of screens.
 *
 * @return The screen count, at least 1.
 * @function count
 */
static int
luaA_screen_count(lua_State *L)
{
    lua_pushinteger(L, globalconf.screens.len);
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
        { "__call", luaA_screen_module_call },
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
    /**
     * @signal .primary_changed
     */
    signal_add(&screen_class.signals, "primary_changed");
    /**
     * This signal is emitted when a new screen is added to the current setup.
     * @signal .added
     */
    signal_add(&screen_class.signals, "added");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
