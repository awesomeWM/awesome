/*
 * systray.c - systray handling
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include "luaa.h"
#include "screen.h"
#include "systray.h"
#include "xwindow.h"
#include "common/array.h"
#include "common/atoms.h"
#include "common/xutil.h"
#include "objects/drawin.h"

#define SYSTEM_TRAY_REQUEST_DOCK 0 /* Begin icon docking */

/** Initialize systray information in X.
 */
void
systray_init(void)
{
    xcb_intern_atom_cookie_t atom_systray_q;
    xcb_intern_atom_reply_t *atom_systray_r;
    char *atom_name;
    xcb_screen_t *xscreen = globalconf.screen;

    globalconf.systray.window = xcb_generate_id(globalconf.connection);
    globalconf.systray.background_pixel = xscreen->black_pixel;
    xcb_create_window(globalconf.connection, xscreen->root_depth,
                      globalconf.systray.window,
                      xscreen->root,
                      -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, xscreen->root_visual,
                      XCB_CW_BACK_PIXEL, (const uint32_t [])
                      { xscreen->black_pixel });

    atom_name = xcb_atom_name_by_screen("_NET_SYSTEM_TRAY", globalconf.default_screen);
    if(!atom_name)
        fatal("error getting systray atom name");

    atom_systray_q = xcb_intern_atom_unchecked(globalconf.connection, false,
                                               a_strlen(atom_name), atom_name);

    p_delete(&atom_name);

    atom_systray_r = xcb_intern_atom_reply(globalconf.connection, atom_systray_q, NULL);
    if(!atom_systray_r)
        fatal("error getting systray atom");

    globalconf.systray.atom = atom_systray_r->atom;
    p_delete(&atom_systray_r);
}

/** Register systray in X.
 */
static void
systray_register(void)
{
    xcb_client_message_event_t ev;
    xcb_screen_t *xscreen = globalconf.screen;

    if(globalconf.systray.registered)
        return;

    globalconf.systray.registered = true;

    /* Fill event */
    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = xscreen->root;
    ev.format = 32;
    ev.type = MANAGER;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = globalconf.systray.atom;
    ev.data.data32[2] = globalconf.systray.window;
    ev.data.data32[3] = ev.data.data32[4] = 0;

    xcb_set_selection_owner(globalconf.connection,
                            globalconf.systray.window,
                            globalconf.systray.atom,
                            XCB_CURRENT_TIME);

    xcb_send_event(globalconf.connection, false, xscreen->root, 0xFFFFFF, (char *) &ev);
}

/** Remove systray information in X.
 */
void
systray_cleanup(void)
{
    if(!globalconf.systray.registered)
        return;

    globalconf.systray.registered = false;

    xcb_set_selection_owner(globalconf.connection,
                            XCB_NONE,
                            globalconf.systray.atom,
                            XCB_CURRENT_TIME);

    xcb_unmap_window(globalconf.connection,
                     globalconf.systray.window);
}

/** Handle a systray request.
 * \param embed_win The window to embed.
 * \param info The embedding info
 * \return 0 on no error.
 */
int
systray_request_handle(xcb_window_t embed_win, xembed_info_t *info)
{
    xembed_window_t em;
    xcb_get_property_cookie_t em_cookie;
    const uint32_t select_input_val[] =
    {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY
            | XCB_EVENT_MASK_PROPERTY_CHANGE
            | XCB_EVENT_MASK_ENTER_WINDOW
    };

    /* check if not already trayed */
    if(xembed_getbywin(&globalconf.embedded, embed_win))
        return -1;

    p_clear(&em_cookie, 1);

    if(!info)
        em_cookie = xembed_info_get_unchecked(globalconf.connection, embed_win);

    xcb_change_window_attributes(globalconf.connection, embed_win, XCB_CW_EVENT_MASK,
                                 select_input_val);

    /* we grab the window, but also make sure it's automatically reparented back
     * to the root window if we should die.
     */
    xcb_change_save_set(globalconf.connection, XCB_SET_MODE_INSERT, embed_win);
    xcb_reparent_window(globalconf.connection, embed_win,
                        globalconf.systray.window,
                        0, 0);

    em.win = embed_win;

    if(info)
        em.info = *info;
    else
        xembed_info_get_reply(globalconf.connection, em_cookie, &em.info);

    xembed_embedded_notify(globalconf.connection, em.win,
                           globalconf.systray.window,
                           MIN(XEMBED_VERSION, em.info.version));

    xembed_window_array_append(&globalconf.embedded, em);
    luaA_systray_invalidate();

    return 0;
}

/** Handle systray message.
 * \param ev The event.
 * \return 0 on no error.
 */
int
systray_process_client_message(xcb_client_message_event_t *ev)
{
    int ret = 0;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;

    switch(ev->data.data32[1])
    {
      case SYSTEM_TRAY_REQUEST_DOCK:
        geom_c = xcb_get_geometry_unchecked(globalconf.connection, ev->window);

        if(!(geom_r = xcb_get_geometry_reply(globalconf.connection, geom_c, NULL)))
            return -1;

        if(globalconf.screen->root == geom_r->root)
            ret = systray_request_handle(ev->data.data32[2], NULL);

        p_delete(&geom_r);
        break;
    }

    return ret;
}

/** Check if a window is a KDE tray.
 * \param w The window to check.
 * \return True if it is, false otherwise.
 */
bool
systray_iskdedockapp(xcb_window_t w)
{
    xcb_get_property_cookie_t kde_check_q;
    xcb_get_property_reply_t *kde_check;
    bool ret;

    /* Check if that is a KDE tray because it does not respect fdo standards,
     * thanks KDE. */
    kde_check_q = xcb_get_property_unchecked(globalconf.connection, false, w,
                                             _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
                                             XCB_ATOM_WINDOW, 0, 1);

    kde_check = xcb_get_property_reply(globalconf.connection, kde_check_q, NULL);

    /* it's a KDE systray ?*/
    ret = (kde_check && kde_check->value_len);

    p_delete(&kde_check);

    return ret;
}

/** Handle xembed client message.
 * \param ev The event.
 * \return 0 on no error.
 */
int
xembed_process_client_message(xcb_client_message_event_t *ev)
{
    switch(ev->data.data32[1])
    {
      case XEMBED_REQUEST_FOCUS:
        xembed_focus_in(globalconf.connection, ev->window, XEMBED_FOCUS_CURRENT);
        break;
    }
    return 0;
}

/** Inform lua that the systray needs to be updated.
 */
void
luaA_systray_invalidate(void)
{
    signal_object_emit(globalconf.L, &global_signals, "systray::update", 0);
}

static void
systray_update(int base_size, bool horizontal, bool reverse, bool force_redraw)
{
    if(base_size <= 0)
        return;

    /* Give the systray window the correct size */
    uint32_t config_vals[4] = { base_size, base_size, 0, 0 };
    if(horizontal)
        config_vals[0] = base_size * globalconf.embedded.len;
    else
        config_vals[1] = base_size * globalconf.embedded.len;
    xcb_configure_window(globalconf.connection,
                         globalconf.systray.window,
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         config_vals);

    /* Now resize each embedded window */
    config_vals[0] = config_vals[1] = 0;
    config_vals[2] = config_vals[3] = base_size;
    for(int i = 0; i < globalconf.embedded.len; i++)
    {
        xembed_window_t *em;

        if(reverse)
            em = &globalconf.embedded.tab[(globalconf.embedded.len - i - 1)];
        else
            em = &globalconf.embedded.tab[i];

        xcb_configure_window(globalconf.connection, em->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             config_vals);
        xcb_map_window(globalconf.connection, em->win);
        if (force_redraw)
            xcb_clear_area(globalconf.connection, 1, em->win, 0, 0, 0, 0);
        if(horizontal)
            config_vals[0] += base_size;
        else
            config_vals[1] += base_size;
    }
}

/** Update the systray
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The drawin to display the systray in.
 * \lparam x X position for the systray.
 * \lparam y Y position for the systray.
 * \lparam base_size The size (width and height) each systray item gets.
 * \lparam horiz If true, the systray is horizontal, else vertical
 * \lparam bg Color of the systray background
 * \lparam revers If true, the systray icon order will be reversed, else default
 */
int
luaA_systray(lua_State *L)
{
    systray_register();

    if(lua_gettop(L) != 0)
    {
        size_t bg_len;
        drawin_t *w = luaA_checkudata(L, 1, &drawin_class);
        int x = luaL_checknumber(L, 2);
        int y = luaL_checknumber(L, 3);
        int base_size = luaL_checknumber(L, 4);
        bool horiz = lua_toboolean(L, 5);
        const char *bg = luaL_checklstring(L, 6, &bg_len);
        bool revers = lua_toboolean(L, 7);
        color_t bg_color;
        bool force_redraw = false;

        if(color_init_reply(color_init_unchecked(&bg_color, bg, bg_len))
                && globalconf.systray.background_pixel != bg_color.pixel)
        {
            uint32_t config_back[] = { bg_color.pixel };
            globalconf.systray.background_pixel = bg_color.pixel;
            xcb_change_window_attributes(globalconf.connection,
                                         globalconf.systray.window,
                                         XCB_CW_BACK_PIXEL, config_back);
            xcb_clear_area(globalconf.connection, 1, globalconf.systray.window, 0, 0, 0, 0);
            force_redraw = true;
        }

        if(globalconf.systray.parent != w)
            xcb_reparent_window(globalconf.connection,
                                globalconf.systray.window,
                                w->window,
                                x, y);
        else
        {
            uint32_t config_vals[2] = { x, y };
            xcb_configure_window(globalconf.connection,
                                 globalconf.systray.window,
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                                 config_vals);
        }

        globalconf.systray.parent = w;

        if(globalconf.embedded.len != 0)
        {
            systray_update(base_size, horiz, revers, force_redraw);
            xcb_map_window(globalconf.connection,
                           globalconf.systray.window);
        }
        else
            xcb_unmap_window(globalconf.connection,
                             globalconf.systray.window);
    }

    lua_pushnumber(L, globalconf.embedded.len);
    luaA_object_push(L, globalconf.systray.parent);
    return 2;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
