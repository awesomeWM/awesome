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
 *    for s in screen do
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
 * @coreclassmod screen
 */

#include "objects/screen.h"
#include "banning.h"
#include "objects/client.h"
#include "objects/drawin.h"

#include <stdio.h>

struct screen_impl screen_impl;

/** AwesomeWM is about to scan for existing screens.
 *
 * Connect to this signal when code needs to be executed after the Lua context
 * is initialized and modules are loaded, but before screens are added.
 *
 * To manage screens manually, set `screen.automatic_factory = false` and
 * connect to the `property::viewports` signal. It is then possible to use
 * `screen.fake_add` to create virtual screens. Be careful when using this,
 * when done incorrectly, no screens will be created. Using Awesome with zero
 * screens is **not** supported.
 *
 * @signal scanning
 * @see property::viewports
 * @see screen.fake_add
 */

/** AwesomeWM is done scanning for screens.
 *
 * Connect to this signal to execute code after the screens have been created,
 * but before the clients are added. This signal can also be used to split
 * physical screens into multiple virtual screens before the clients (and their
 * rules) are executed.
 *
 * Note that if no screens exist at this point, the fallback code will be
 * triggered and the default (detected) screens will be added.
 *
 * @signal scanned
 * @see screen.fake_resize
 * @see screen.fake_add
 */

/** Screen is a table where indexes are screen numbers. You can use `screen[1]`
 * to get access to the first screen, etc. Alternatively, if RANDR information
 * is available, you can use output names for finding screen objects.
 * The primary screen can be accessed as `screen.primary`.
 * Each screen has a set of properties.
 *
 */

/**
 * @signal primary_changed
 */

/**
 * This signal is emitted when a new screen is added to the current setup.
 * @signal added
 */

/**
 * This signal is emitted when a screen is removed from the setup.
 * @signal removed
 */

/** This signal is emitted when the list of available screens changes.
 * @signal list
 */

/** When 2 screens are swapped
 * @tparam screen screen The other screen
 * @tparam boolean is_source If self is the source or the destination of the swap
 * @signal swapped
 */

/** This signal is emitted when the list of physical screen viewport changes.
 *
 * Each viewport in the list corresponds to a **physical** screen rectangle, which
 * is **not** the `viewports` property of the `screen` objects.
 *
 * Each entry in the `viewports` entry has the following keys:
 *
 * * `geometry` *(table)*: A table with an `x`, `y`, `width` and `height` keys.
 * * `outputs` *(table)*: All outputs sharing this viewport.
 * * `maximum_dpi` *(number)*: The DPI of the most dense output.
 * * `minimum_dpi` *(number)*: The DPI of the least dense output.
 * * `preferred_dpi` *(number)*: The optimal DPI.
 *
 * @signal property::viewports
 * @tparam table viewports A table containing all physical viewports.
 * @see automatic_factory
 */

 /**
  * The primary screen.
  *
  * @tfield screen primary
  */

/**
 * If `screen` objects are created automatically when new viewports are detected.
 *
 * Be very, very careful when setting this to false. You might end up with
 * no screens. This is **not** supported. Always connect to the `scanned`
 * signal to make sure to create a fallback screen if none were created.
 *
 * @tfield[opt=true] boolean screen.automatic_factory
 * @see property::viewports
 */

/**
 * The screen coordinates.
 *
 * **Signal:**
 *
 *  * *property::geometry*
 *
 * **Immutable:** true
 *
 * @DOC_screen_geometry_EXAMPLE@
 *
 * @property geometry
 * @param table
 * @tfield integer table.x The horizontal position
 * @tfield integer table.y The vertical position
 * @tfield integer table.width The width
 * @tfield integer table.height The height
 */

/**
 * The internal screen number.
 *
 * * The indeces are a continuous sequence from 1 to `screen.count()`.
 * * It is **NOT** related to the actual screen position relative to each
 *   other.
 * * 1 is **NOT** necessarily the primary screen.
 * * When screens are added and removed indices **CAN** change.
 *
 * If you really want to keep an array of screens you should use something
 * along:
 *
 *     local myscreens = setmetatable({}. {__mode="k"})
 *     myscreens[ screen[1] ] = "mydata"
 *
 * But it might be a better option to simply store the data directly in the
 * screen object as:
 *
 *     screen[1].mydata = "mydata"
 *
 * Remember that screens are also objects, so if you only want to store a simple
 * property, you can do it directly:
 *
 *     screen[1].answer = 42
 *
 * **Immutable:** true
 * @property index
 * @param integer
 * @see screen
 */

/**
 * The screen workarea.
 *
 * The workarea is a subsection of the screen where clients can be placed. It
 * usually excludes the toolbars (see `awful.wibar`) and dockable clients
 * (see `client.dockable`) like WindowMaker DockAPP.
 *
 * It can be modified be altering the `wibox` or `client` struts.
 *
 * **Signal:**
 *
 *  * *property::workarea*
 *
 * @DOC_screen_workarea_EXAMPLE@
 *
 * @property workarea
 * @see client.struts
 * @param table
 * @tfield integer table.x The horizontal position
 * @tfield integer table.y The vertical position
 * @tfield integer table.width The width
 * @tfield integer table.height The height
 */


/** Get the number of instances.
 *
 * @return The number of screen objects alive.
 * @staticfct instances
 */

/* Set a __index metamethod for all screen instances.
 * @tparam function cb The meta-method
 * @staticfct set_index_miss_handler
 */

/* Set a __newindex metamethod for all screen instances.
 * @tparam function cb The meta-method
 * @staticfct set_newindex_miss_handler
 */

lua_class_t screen_class;
LUA_OBJECT_FUNCS(screen_class, screen_t, screen)

/** Collect a screen. */
static void
screen_wipe(screen_t *s)
{
    if (s->name) {
        free(s->name);
        s->name = NULL;
    }

    screen_impl.wipe_screen(s);
}

/** Check if a screen is valid */
static bool
screen_checker(screen_t *s)
{
    return s->valid;
}

/** Get a screen argument from the lua stack */
screen_t *
luaA_checkscreen(lua_State *L, int sidx)
{
    if (lua_isnumber(L, sidx))
    {
        int screen = lua_tointeger(L, sidx);
        if(screen < 1 || screen > globalconf.screens.len)
        {
            luaA_warn(L, "invalid screen number: %d (of %d existing)", screen, globalconf.screens.len);
            lua_pushnil(L);
            return NULL;
        }
        return globalconf.screens.tab[screen - 1];
    } else
        return luaA_checkudata(L, sidx, &screen_class);
}

void screen_deduplicate(lua_State *L, screen_array_t *screens)
{
    /* Remove duplicate screens */
    for(int first = 0; first < screens->len; first++) {
        screen_t *first_screen = screens->tab[first];
        for(int second = 0; second < screens->len; second++) {
            screen_t *second_screen = screens->tab[second];
            if (first == second)
                continue;

            if (first_screen->geometry.width < second_screen->geometry.width
                    && first_screen->geometry.height < second_screen->geometry.height)
                /* Don't drop a smaller screen */
                continue;

            if (first_screen->geometry.x == second_screen->geometry.x
                    && first_screen->geometry.y == second_screen->geometry.y) {
                /* Found a duplicate */
                first_screen->geometry.width = MAX(first_screen->geometry.width, second_screen->geometry.width);
                first_screen->geometry.height = MAX(first_screen->geometry.height, second_screen->geometry.height);

                screen_array_take(screens, second);
                luaA_object_unref(L, second_screen);

                /* Restart the search */
                screen_deduplicate(L, screens);
                return;
            }
        }
    }
}


int luaA_viewports(lua_State *L)
{
    return screen_impl.get_viewports(L);
}

screen_t * screen_add(lua_State *L, screen_array_t *screens)
{
    screen_t *new_screen = screen_new(L);
    luaA_object_ref(L, -1);
    screen_array_append(screens, new_screen);
    screen_impl.new_screen(new_screen);
    return new_screen;
}

void screen_added(lua_State *L, screen_t *screen)
{
    screen->workarea = screen->geometry;
    screen->valid = true;
    luaA_object_push(L, screen);
    luaA_object_emit_signal(L, -1, "_added", 0);
    lua_pop(L, 1);
}

void
screen_emit_scanned(void)
{
    lua_State *L = globalconf_get_lua_State();
    luaA_class_emit_signal(L, &screen_class, "scanned", 0);
}

void
screen_emit_scanning(void)
{
    lua_State *L = globalconf_get_lua_State();
    luaA_class_emit_signal(L, &screen_class, "scanning", 0);
}

static void
screen_scan_common(bool quiet)
{
    screen_impl.scan_screens();
}

/** Get screens informations and fill global configuration.
 */
void
screen_scan(void)
{
    screen_emit_scanning();
    screen_scan_common(false);
}

static int
luaA_scan_quiet(lua_State *L)
{
    screen_scan_common(true);
    return 0;
}

/* Called when a screen is removed, removes references to the old screen */
static void
screen_removed(lua_State *L, int sidx)
{
    screen_t *screen = luaA_checkudata(L, sidx, &screen_class);

    luaA_object_emit_signal(L, sidx, "removed", 0);

    if (globalconf.primary_screen == screen)
        globalconf.primary_screen = NULL;

    foreach(c, globalconf.clients) {
        if((*c)->screen == screen)
            screen_client_moveto(*c, screen_getbycoord(
                        (*c)->geometry.x, (*c)->geometry.y), false);
    }
}

void screen_cleanup(void)
{
    while(globalconf.screens.len)
        screen_array_take(&globalconf.screens, 0);

    screen_impl.cleanup_screens();
}

static void
screen_modified(screen_t *existing_screen, screen_t *other_screen)
{
    lua_State *L = globalconf_get_lua_State();

    if(!AREA_EQUAL(existing_screen->geometry, other_screen->geometry)) {
        area_t old_geometry = existing_screen->geometry;
        existing_screen->geometry = other_screen->geometry;
        luaA_object_push(L, existing_screen);
        luaA_pusharea(L, old_geometry);
        luaA_object_emit_signal(L, -2, "property::geometry", 1);
        lua_pop(L, 1);
        screen_update_workarea(existing_screen);
    }

    bool outputs_changed = screen_impl.outputs_changed(existing_screen, other_screen);
    if(outputs_changed) {
        luaA_object_push(L, existing_screen);
        luaA_object_emit_signal(L, -1, "property::_outputs", 0);
        lua_pop(L, 1);
    }
}

static gboolean
screen_refresh(gpointer unused)
{
    globalconf.screen_refresh_pending = false;

    screen_array_t new_screens;
    screen_array_t removed_screens;
    lua_State *L = globalconf_get_lua_State();
    bool list_changed = false;

    screen_array_init(&new_screens);
    screen_impl.get_screens(L, &new_screens);

    /* Add new screens */
    foreach(new_screen, new_screens) {
        bool found =
            screen_impl.does_screen_exist(*new_screen, globalconf.screens);
        if(!found) {
            screen_array_append(&globalconf.screens, *new_screen);
            screen_added(L, *new_screen);
            /* Get an extra reference since both new_screens and
             * globalconf.screens reference this screen now */
            luaA_object_push(L, *new_screen);
            luaA_object_ref(L, -1);

            list_changed = true;
        }
    }

    /* Remove screens which are gone */
    screen_array_init(&removed_screens);
    for(int i = 0; i < globalconf.screens.len; i++) {
        screen_t *old_screen = globalconf.screens.tab[i];
        bool found = screen_impl.is_fake_screen(old_screen)
            || screen_impl.does_screen_exist(old_screen, new_screens);
        if(old_screen->lifecycle & SCREEN_LIFECYCLE_C && !found) {
            screen_array_take(&globalconf.screens, i);
            i--;

            screen_array_append(&removed_screens, old_screen);
            list_changed = true;
        }
    }
    foreach(old_screen, removed_screens) {
        luaA_object_push(L, *old_screen);
        screen_removed(L, -1);
        lua_pop(L, 1);
        (*old_screen)->valid = false;
        luaA_object_unref(L, *old_screen);
    }
    screen_array_wipe(&removed_screens);

    /* Update changed screens */
    foreach(existing_screen, globalconf.screens)
        foreach(new_screen, new_screens)
            if(screen_impl.is_same_screen(*existing_screen, *new_screen))
                screen_modified(*existing_screen, *new_screen);

    foreach(screen, new_screens)
        luaA_object_unref(L, *screen);
    screen_array_wipe(&new_screens);

    screen_update_primary();

    if (list_changed)
        luaA_class_emit_signal(L, &screen_class, "list", 0);

    return G_SOURCE_REMOVE;
}

void
screen_schedule_refresh(void)
{
    if(globalconf.screen_refresh_pending || !globalconf.have_randr_13)
        return;

    globalconf.screen_refresh_pending = true;
    g_idle_add_full(G_PRIORITY_LOW, screen_refresh, NULL, NULL);
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

/** Is there any overlap between the given geometry and a given screen?
 * \param screen The logical screen number.
 * \param geom The geometry
 * \return True if there is any overlap between the geometry and a given screen.
 */
bool
screen_area_in_screen(screen_t *s, area_t geom)
{
    return (geom.x < s->geometry.x + s->geometry.width)
            && (geom.x + geom.width > s->geometry.x )
            && (geom.y < s->geometry.y + s->geometry.height)
            && (geom.y + geom.height > s->geometry.y);
}

void screen_update_workarea(screen_t *screen)
{
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
    area.width -= MIN(area.width, left + right);
    area.height -= MIN(area.height, top + bottom);

    if (AREA_EQUAL(area, screen->workarea))
        return;

    area_t old_workarea = screen->workarea;
    screen->workarea = area;
    lua_State *L = globalconf_get_lua_State();
    luaA_object_push(L, screen);
    luaA_pusharea(L, old_workarea);
    luaA_object_emit_signal(L, -2, "property::workarea", 1);
    lua_pop(L, 1);
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

    from = old_screen->geometry;
    to = c->screen->geometry;

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
    if(!screen_area_in_screen(new_screen, new_geometry))
    {
        /* If all else fails, force the client to end up on screen. */
        new_geometry.x = to.x;
        new_geometry.y = to.y;
    }

    /* move / resize the client */
    client_resize(c, new_geometry, false);

    /* emit signal */
    luaA_object_push(L, c);
    if(old_screen != NULL)
        luaA_object_push(L, old_screen);
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

void
screen_update_primary(void)
{
    screen_t *primary_screen = screen_impl.update_primary();
    if (primary_screen == NULL)
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
    {
        globalconf.primary_screen = globalconf.screens.tab[0];

        lua_State *L = globalconf_get_lua_State();
        luaA_object_push(L, globalconf.primary_screen);
        luaA_object_emit_signal(L, -1, "primary_changed", 0);
        lua_pop(L, 1);
    }
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
    char *name;

    if(lua_type(L, 2) == LUA_TSTRING && (name = (char *) lua_tostring(L, 2)))
    {
        if(A_STREQ(name, "primary"))
            return luaA_object_push(L, screen_get_primary());
        else if (A_STREQ(name, "automatic_factory"))
        {
            lua_pushboolean(L, !globalconf.ignore_screens);
            return 1;
        }

        screen_t *screen = screen_impl.screen_by_name(name);
        if (screen == NULL)
        {
            luaA_warn(L, "Unknown screen output name: %s", name);
            lua_pushnil(L);
            return 1;
        }
        return luaA_object_push(L, screen);
    }

    return luaA_object_push(L, luaA_checkscreen(L, 2));
}

static int
luaA_screen_module_newindex(lua_State *L)
{
    const char *buf = luaL_checkstring(L, 2);

    if (A_STREQ(buf, "automatic_factory"))
    {
        globalconf.ignore_screens = !luaA_checkboolean(L, 3);

        /* It *can* be useful if screens are added/removed later, but generally,
         * setting this should be done before screens are added
         */
        if (globalconf.ignore_screens && !globalconf.no_auto_screen)
            luaA_warn(L,
                "Setting automatic_factory only makes sense when AwesomeWM is"
                " started with `--screen off`"
            );
    }

    return luaA_default_newindex(L);
}

/** Iterate over screens.
 * @usage
 * for s in screen do
 *     print("Oh, wow, we have screen " .. tostring(s))
 * end
 * @staticfct screen
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
    return screen_impl.viewport_get_outputs(L, s->viewport);
}

static int
luaA_screen_get_managed(lua_State *L, screen_t *s)
{
    if (s->lifecycle & SCREEN_LIFECYCLE_LUA)
        lua_pushstring(L, "Lua");
    else if (s->lifecycle & SCREEN_LIFECYCLE_C)
        lua_pushstring(L, "C");
    else
        lua_pushstring(L, "none");

    return 1;
}

static int
luaA_screen_get_workarea(lua_State *L, screen_t *s)
{
    luaA_pusharea(L, s->workarea);
    return 1;
}

static int
luaA_screen_set_name(lua_State *L, screen_t *s)
{
    const char *buf = luaL_checkstring(L, -1);

    if (s->name)
        free(s->name);

    s->name = a_strdup(buf);

    return 0;
}

static int
luaA_screen_get_name(lua_State *L, screen_t *s)
{
    lua_pushstring(L, s->name ? s->name : "screen");

    /* Fallback to "screen1", "screen2", etc if no name is set */
    if (!s->name) {
        lua_pushinteger(L, screen_get_index(s));
        lua_concat(L, 2);
    }

    return 1;
}

/** Get the number of screens.
 *
 * @return The screen count, at least 1.
 * @staticfct count
 */
static int
luaA_screen_count(lua_State *L)
{
    lua_pushinteger(L, globalconf.screens.len);
    return 1;
}

/** Add a fake screen.
 *
 * To vertically split the first screen in 2 equal parts, use:
 *
 *    local geo = screen[1].geometry
 *    local new_width = math.ceil(geo.width/2)
 *    local new_width2 = geo.width - new_width
 *    screen[1]:fake_resize(geo.x, geo.y, new_width, geo.height)
 *    screen.fake_add(geo.x + new_width, geo.y, new_width2, geo.height)
 *
 * Both virtual screens will have their own taglist and wibars.
 *
 * @tparam integer x X-coordinate for screen.
 * @tparam integer y Y-coordinate for screen.
 * @tparam integer width width for screen.
 * @tparam integer height height for screen.
 * @return The new screen.
 * @constructorfct fake_add
 */
static int
luaA_screen_fake_add(lua_State *L)
{
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int width = luaL_checkinteger(L, 3);
    int height = luaL_checkinteger(L, 4);

    /* If the screen is managed by internal Lua code */
    bool managed = false;

    /* Allow undocumented arguments for internal use only */
    if(lua_istable(L, 5)) {
        lua_getfield(L, 5, "_managed");
        managed = lua_isboolean(L, 6) && luaA_checkboolean(L, 6);

        lua_pop(L, 1);
    }

    screen_t *s;

    s = screen_add(L, &globalconf.screens);
    s->lifecycle |= managed ? SCREEN_LIFECYCLE_LUA : SCREEN_LIFECYCLE_USER;
    s->geometry.x = x;
    s->geometry.y = y;
    s->geometry.width = width;
    s->geometry.height = height;

    screen_impl.mark_fake_screen(s);

    screen_added(L, s);
    luaA_class_emit_signal(L, &screen_class, "list", 0);
    luaA_object_push(L, s);

    foreach(c, globalconf.clients) {
        screen_client_moveto(*c, screen_getbycoord(
                    (*c)->geometry.x, (*c)->geometry.y), false);
    }

    return 1;
}

/** Remove a screen.
 * @method fake_remove
 */
static int
luaA_screen_fake_remove(lua_State *L)
{
    screen_t *s = luaA_checkudata(L, 1, &screen_class);
    int idx = screen_get_index(s) - 1;
    if (idx < 0)
        /* WTF? */
        return 0;

    if (globalconf.screens.len == 1) {
        luaA_warn(L, "Removing last screen through fake_remove(). "
                "This is a very, very, very bad idea!");
    }

    screen_array_take(&globalconf.screens, idx);
    luaA_object_push(L, s);
    screen_removed(L, -1);
    lua_pop(L, 1);
    luaA_class_emit_signal(L, &screen_class, "list", 0);
    luaA_object_unref(L, s);
    s->valid = false;

    return 0;
}

/** Fake-resize a screen
 * @tparam integer x The new X-coordinate for screen.
 * @tparam integer y The new Y-coordinate for screen.
 * @tparam integer width The new width for screen.
 * @tparam integer height The new height for screen.
 * @method fake_resize
 */
static int
luaA_screen_fake_resize(lua_State *L)
{
    screen_t *screen = luaA_checkudata(L, 1, &screen_class);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int width = luaL_checkinteger(L, 4);
    int height = luaL_checkinteger(L, 5);
    area_t old_geometry = screen->geometry;

    screen->geometry.x = x;
    screen->geometry.y = y;
    screen->geometry.width = width;
    screen->geometry.height = height;

    screen_update_workarea(screen);

    luaA_pusharea(L, old_geometry);
    luaA_object_emit_signal(L, 1, "property::geometry", 1);

    /* Note: calling `screen_client_moveto` from here will create more issues
     * than it would fix. Keep in mind that it means `c.screen` will be wrong
     * until Lua it `fake_add` fixes it.
     */

    return 0;
}

/** Swap a screen with another one in global screen list.
 * @client s A screen to swap with.
 * @method swap
 */
static int
luaA_screen_swap(lua_State *L)
{
    screen_t *s = luaA_checkudata(L, 1, &screen_class);
    screen_t *swap = luaA_checkudata(L, 2, &screen_class);

    if(s != swap)
    {
        screen_t **ref_s = NULL, **ref_swap = NULL;
        foreach(item, globalconf.screens)
        {
            if(*item == s)
                ref_s = item;
            else if(*item == swap)
                ref_swap = item;
            if(ref_s && ref_swap)
                break;
        }
        if(!ref_s || !ref_swap)
            return luaL_error(L, "Invalid call to screen:swap()");

        /* swap ! */
        *ref_s = swap;
        *ref_swap = s;

        luaA_class_emit_signal(L, &screen_class, "list", 0);

        luaA_object_push(L, swap);
        lua_pushboolean(L, true);
        luaA_object_emit_signal(L, -4, "swapped", 2);

        luaA_object_push(L, swap);
        luaA_object_push(L, s);
        lua_pushboolean(L, false);
        luaA_object_emit_signal(L, -3, "swapped", 2);
    }

    return 0;
}

void
screen_class_setup(lua_State *L)
{
    static const struct luaL_Reg screen_methods[] =
    {
        LUA_CLASS_METHODS(screen)
        { "count", luaA_screen_count },
        { "_viewports", luaA_viewports },
        { "_scan_quiet", luaA_scan_quiet },
        { "__index", luaA_screen_module_index },
        { "__newindex", luaA_screen_module_newindex },
        { "__call", luaA_screen_module_call },
        { "fake_add", luaA_screen_fake_add },
        { NULL, NULL }
    };

    static const struct luaL_Reg screen_meta[] =
    {
        LUA_OBJECT_META(screen)
        LUA_CLASS_META
        { "fake_remove", luaA_screen_fake_remove },
        { "fake_resize", luaA_screen_fake_resize },
        { "swap", luaA_screen_swap },
        { NULL, NULL },
    };

    luaA_class_setup(L, &screen_class, "screen", NULL,
                     (lua_class_allocator_t) screen_new,
                     (lua_class_collector_t) screen_wipe,
                     (lua_class_checker_t) screen_checker,
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
    luaA_class_add_property(&screen_class, "_outputs",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_outputs,
                            NULL);
    luaA_class_add_property(&screen_class, "_managed",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_managed,
                            NULL);
    luaA_class_add_property(&screen_class, "workarea",
                            NULL,
                            (lua_class_propfunc_t) luaA_screen_get_workarea,
                            NULL);
    luaA_class_add_property(&screen_class, "name",
                            (lua_class_propfunc_t) luaA_screen_set_name,
                            (lua_class_propfunc_t) luaA_screen_get_name,
                            (lua_class_propfunc_t) luaA_screen_set_name);
}

/* @DOC_cobject_COMMON@ */

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
