/*
 * event.c - event handlers
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

#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include <xcb/shape.h>
#include <xcb/randr.h>

#include "screen.h"
#include "event.h"
#include "tag.h"
#include "statusbar.h"
#include "window.h"
#include "mouse.h"
#include "ewmh.h"
#include "client.h"
#include "widget.h"
#include "rules.h"
#include "titlebar.h"
#include "layouts/tile.h"
#include "layouts/floating.h"
#include "common/xscreen.h"
#include "common/xutil.h"

extern AwesomeConf globalconf;

/** Handle mouse button click
 * \param screen screen number
 * \param button button number
 * \param state modkeys state
 * \param buttons buttons list to check for
 * \param arg optional arg passed to uicb, otherwise buttons' arg are used
 */
static void
event_handle_mouse_button_press(int screen, unsigned int button,
                                unsigned int state,
                                Button *buttons, char *arg)
{
    Button *b;

    for(b = buttons; b; b = b->next)
        if(button == b->button && CLEANMASK(state) == b->mod && b->func)
        {
            if(arg)
                b->func(screen, arg);
            else
                b->func(screen, b->arg);
        }
}

/** Handle XButtonPressed events
 * \param connection connection to the X server
 * \param ev ButtonPress event
 */
int
event_handle_buttonpress(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection, xcb_button_press_event_t *ev)
{
    int screen;
    client_t *c;
    widget_t *widget;
    statusbar_t *statusbar;
    xcb_query_pointer_cookie_t qc;
    xcb_query_pointer_reply_t *qr;

    qc = xcb_query_pointer(connection, ev->event);

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
            if(statusbar->sw->window == ev->event || statusbar->sw->window == ev->child)
            {
                switch(statusbar->position)
                {
                  case Top:
                  case Bottom:
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(ev->event_x >= widget->area.x && ev->event_x < widget->area.x + widget->area.width
                           && ev->event_y >= widget->area.y && ev->event_y < widget->area.y + widget->area.height)
                        {
                            widget->button_press(widget, ev);
                            return 0;
                        }
                    break;
                  case Right:
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(ev->event_y >= widget->area.x && ev->event_y < widget->area.x + widget->area.width
                           && statusbar->sw->geometry.width - ev->event_x >= widget->area.y
                           && statusbar->sw->geometry.width - ev->event_x
                              < widget->area.y + widget->area.height)
                        {
                            widget->button_press(widget, ev);
                            return 0;
                        }
                    break;
                  case Left:
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(statusbar->sw->geometry.height - ev->event_y >= widget->area.x
                           && statusbar->sw->geometry.height - ev->event_y
                              < widget->area.x + widget->area.width
                           && ev->event_x >= widget->area.y
                           && ev->event_x < widget->area.y + widget->area.height)
                        {
                            widget->button_press(widget, ev);
                            return 0;
                        }
                    break;
                  default:
                    break;
                }
                /* return if no widget match */
                return 0;
            }

    /* Check for titlebar first */
    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar.sw && c->titlebar.sw->window == ev->event)
        {
            if(!client_focus(c, c->screen, true))
                client_stack(c);
            if(CLEANMASK(ev->state) == XCB_NO_SYMBOL
               && ev->detail == XCB_BUTTON_INDEX_1)
                window_grabbuttons(c->win, c->phys_screen);
            event_handle_mouse_button_press(c->screen, ev->detail, ev->state,
                                            globalconf.buttons.titlebar, NULL);
            return 0;
        }

    if((c = client_get_bywin(globalconf.clients, ev->event)))
    {
        if(!client_focus(c, c->screen, true))
            client_stack(c);
        if(CLEANMASK(ev->state) == XCB_NO_SYMBOL
           && ev->detail == XCB_BUTTON_INDEX_1)
        {
            xcb_allow_events(globalconf.connection, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);
            window_grabbuttons(c->win, c->phys_screen);
        }
        else
            event_handle_mouse_button_press(c->screen, ev->detail, ev->state, globalconf.buttons.client, NULL);
    }
    else
        for(screen = 0; screen < xcb_setup_roots_length(xcb_get_setup(connection)); screen++)
            if(xcb_aux_get_screen(connection, screen)->root == ev->event
               && (qr = xcb_query_pointer_reply(connection, qc, NULL)))
            {
                screen = screen_get_bycoord(globalconf.screens_info, screen, qr->root_x, qr->root_y);
                event_handle_mouse_button_press(screen, ev->detail, ev->state,
                                                globalconf.buttons.root, NULL);

                p_delete(&qr);
                return 0;
            }

    return 0;
}

/** Handle XConfigureRequest events
 * \param connection connection to the X server
 * \param ev ConfigureRequest event
 */
int
event_handle_configurerequest(void *data __attribute__ ((unused)),
                              xcb_connection_t *connection, xcb_configure_request_event_t *ev)
{
    client_t *c;
    area_t geometry;

    if((c = client_get_bywin(globalconf.clients, ev->window)))
    {
        geometry = c->geometry;

        if(ev->value_mask & XCB_CONFIG_WINDOW_X)
            geometry.x = ev->x;
        if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
            geometry.y = ev->y;
        if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
            geometry.width = ev->width;
        if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
            geometry.height = ev->height;

        if(geometry.x != c->geometry.x || geometry.y != c->geometry.y
           || geometry.width != c->geometry.width || geometry.height != c->geometry.height)
        {
            if(c->isfloating || layout_get_current(c->screen)->arrange == layout_floating)
                client_resize(c, geometry, false);
            else
            {
                globalconf.screens[c->screen].need_arrange = true;
                /* If we do not resize the client, at least tell it that it
                 * has its new configuration. That fixes at least
                 * gnome-terminal */
                window_configure(c->win, c->geometry, c->border);
            }
        }
        else
        {
            titlebar_update_geometry_floating(c);
            window_configure(c->win, geometry, c->border);
        }
    }
    else
    {
        uint16_t config_win_mask = 0;
        uint32_t config_win_vals[7];
        unsigned short i = 0;

        if(ev->value_mask & XCB_CONFIG_WINDOW_X)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_X;
            config_win_vals[i++] = ev->x;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_Y;
            config_win_vals[i++] = ev->y;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_WIDTH;
            config_win_vals[i++] = ev->width;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_HEIGHT;
            config_win_vals[i++] = ev->height;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
            config_win_vals[i++] = ev->border_width;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_SIBLING)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_SIBLING;
            config_win_vals[i++] = ev->sibling;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
        {
            config_win_mask |= XCB_CONFIG_WINDOW_STACK_MODE;
            config_win_vals[i++] = ev->stack_mode;
        }

        xcb_configure_window(connection, ev->window, config_win_mask, config_win_vals);
    }

    return 0;
}

/** Handle XConfigure events
 * \param connection connection to the X server
 * \param ev ConfigureNotify event
 */
int
event_handle_configurenotify(void *data __attribute__ ((unused)),
                             xcb_connection_t *connection, xcb_configure_notify_event_t *ev)
{
    int screen_nbr;
    const xcb_screen_t *screen;

    for(screen_nbr = 0; screen_nbr < xcb_setup_roots_length(xcb_get_setup (connection)); screen_nbr++)
        if((screen = xcb_aux_get_screen(connection, screen_nbr)) != NULL
           && ev->window == screen->root
           && (ev->width != screen->width_in_pixels
               || ev->height != screen->height_in_pixels))
            /* it's not that we panic, but restart */
            uicb_restart(0, NULL);

    return 0;
}

/** Handle XDestroyWindow events
 * \param connection connection to the X server
 * \param ev DestroyNotify event
 */
int
event_handle_destroynotify(void *data __attribute__ ((unused)),
                           xcb_connection_t *connection __attribute__ ((unused)),
                           xcb_destroy_notify_event_t *ev)
{
    client_t *c;

    if((c = client_get_bywin(globalconf.clients, ev->window)))
        client_unmanage(c);

    return 0;
}

/** Handle XCrossing events
 * \param connection connection to the X server
 * \param ev Crossing event
 */
int
event_handle_enternotify(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection, xcb_enter_notify_event_t *ev)
{
    client_t *c;
    int screen;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL
       || (ev->root_x == globalconf.pointer_x
           && ev->root_y == globalconf.pointer_y))
        return 0;

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar.sw && c->titlebar.sw->window == ev->event)
            break;

    if(c || (c = client_get_bywin(globalconf.clients, ev->event)))
    {
        window_grabbuttons(c->win, c->phys_screen);
        /* the idea behind saving pointer_x and pointer_y is Bob Marley powered
         * this will allow us top drop some EnterNotify events and thus not giving
         * focus to windows appering under the cursor without a cursor move */
        globalconf.pointer_x = ev->root_x;
        globalconf.pointer_y = ev->root_y;

        if(globalconf.screens[c->screen].sloppy_focus)
            client_focus(c, c->screen,
                         globalconf.screens[c->screen].sloppy_focus_raise);
    }
    else
        for(screen = 0; screen < xcb_setup_roots_length(xcb_get_setup(connection)); screen++)
            if(ev->event == xcb_aux_get_screen(connection, screen)->root)
            {
                window_root_grabbuttons(screen);
                return 0;
            }

    return 0;
}

/** Handle XExpose events
 * \param ev Expose event
 */
int
event_handle_expose(void *data __attribute__ ((unused)),
                    xcb_connection_t *connection __attribute__ ((unused)),
                    xcb_expose_event_t *ev)
{
    int screen;
    statusbar_t *statusbar;
    client_t *c;

    if(!ev->count)
    {
        for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
            for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
                if(statusbar->sw->window == ev->window
                   && statusbar->position)
                {
                    simplewindow_refresh_drawable(statusbar->sw);
                    return 0;
                }

        for(c = globalconf.clients; c; c = c->next)
            if(c->titlebar.sw && c->titlebar.sw->window == ev->window)
            {
                simplewindow_refresh_drawable(c->titlebar.sw);
                return 0;
            }
    }

    return 0;
}

/** Handle XKey events
 * \param connection connection to the X server
 * \param ev KeyPress event
 */
int
event_handle_keypress(void *data __attribute__ ((unused)),
                      xcb_connection_t *connection, xcb_key_press_event_t *ev)
{
    int screen;
    xcb_query_pointer_reply_t *qpr = NULL;
    xcb_keysym_t keysym;
    Key *k;

    /* find the right screen for this event */
    for(screen = 0; screen < xcb_setup_roots_length (xcb_get_setup (connection)); screen++)
        if((qpr = xcb_query_pointer_reply(connection,
                                          xcb_query_pointer(connection,
                                                            xcb_aux_get_screen(connection, screen)->root),
                                          NULL)) != NULL)
        {
            /* if screen is 0, we are on first Zaphod screen or on the
             * only screen in Xinerama, so we can ask for a better screen
             * number with screen_get_bycoord: we'll get 0 in Zaphod mode
             * so it's the same, or maybe the real Xinerama screen */
            screen = screen_get_bycoord(globalconf.screens_info, screen, qpr->root_x, qpr->root_y);
            p_delete(&qpr);
            break;
        }

    keysym = xcb_key_symbols_get_keysym(globalconf.keysyms, ev->detail, 0);

    for(k = globalconf.keys; k; k = k->next)
        if(((k->keycode && ev->detail == k->keycode) || (k->keysym && keysym == k->keysym))
           && k->func && CLEANMASK(k->mod) == CLEANMASK(ev->state))
            k->func(screen, k->arg);

    return 0;
}

/** Handle XMapping events
 * \param connection connection to the X server
 * \param ev MappingNotify event
 */
int
event_handle_mappingnotify(void *data __attribute__ ((unused)),
                           xcb_connection_t *connection, xcb_mapping_notify_event_t *ev)
{
    int screen;

    xcb_refresh_keyboard_mapping(globalconf.keysyms, ev);
    if(ev->request == XCB_MAPPING_KEYBOARD)
        for(screen = 0; screen < xcb_setup_roots_length(xcb_get_setup(connection)); screen++)
            window_root_grabkeys(screen);

    return 0;
}

/** Handle XMapRequest events
 * \param connection connection to the X server
 * \param ev MapRequest event
 */
int
event_handle_maprequest(void *data __attribute__ ((unused)),
                        xcb_connection_t *connection, xcb_map_request_event_t *ev)
{
    int screen_nbr = 0;
    xcb_get_window_attributes_cookie_t wa_c;
    xcb_get_window_attributes_reply_t *wa_r;
    xcb_query_pointer_cookie_t qp_c;
    xcb_query_pointer_reply_t *qp_r = NULL;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;
    xcb_screen_iterator_t iter;

    wa_c = xcb_get_window_attributes(connection, ev->window);
    geom_c = xcb_get_geometry(connection, ev->window);
    qp_c = xcb_query_pointer(connection, xcb_aux_get_screen(globalconf.connection, screen_nbr)->root);

    if(!(wa_r = xcb_get_window_attributes_reply(connection, wa_c, NULL)))
        return -1;

    if(wa_r->override_redirect)
    {
        p_delete(&wa_r);
        return 0;
    }

    p_delete(&wa_r);

    if(!client_get_bywin(globalconf.clients, ev->window))
    {
        if(!(geom_r = xcb_get_geometry_reply(connection, geom_c, NULL)))
            return -1;

        if(globalconf.screens_info->xinerama_is_active
           && (qp_r = xcb_query_pointer_reply(connection, qp_c, NULL)))
        {
            screen_nbr = screen_get_bycoord(globalconf.screens_info, screen_nbr,
                                            qp_r->root_x, qp_r->root_y);
            p_delete(&qp_r);
        }
        else
            for(iter = xcb_setup_roots_iterator(xcb_get_setup(connection)), screen_nbr = 0;
                iter.rem && iter.data->root != geom_r->root; xcb_screen_next (&iter), ++screen_nbr);

        client_manage(ev->window, geom_r, screen_nbr);
        p_delete(&geom_r);
    }

    return 0;
}

/** Handle XProperty events
 * \param connection connection to the X server
 * \param ev PropertyNotify event
 */
int
event_handle_propertynotify(void *data __attribute__ ((unused)),
                            xcb_connection_t *connection, xcb_property_notify_event_t *ev)
{
    client_t *c;
    xcb_window_t trans;

    if(ev->state == XCB_PROPERTY_DELETE)
        return 0; /* ignore */
    if((c = client_get_bywin(globalconf.clients, ev->window)))
    {
        if(ev->atom == WM_TRANSIENT_FOR)
        {
            xutil_get_transient_for_hint(connection, c->win, &trans);
            if(!c->isfloating
               && (c->isfloating = (client_get_bywin(globalconf.clients, trans) != NULL)))
                globalconf.screens[c->screen].need_arrange = true;
        }
        else if (ev->atom == WM_NORMAL_HINTS)
            client_updatesizehints(c);
        else if (ev->atom == WM_HINTS)
            client_updatewmhints(c);

        if(ev->atom == WM_NAME
           || ev->atom == xutil_intern_atom(globalconf.connection, "_NET_WM_NAME"))
            client_updatetitle(c);
    }

    return 0;
}

/** Handle XUnmap events
 * \param connection connection to the X server
 * \param ev UnmapNotify event
 */
int
event_handle_unmapnotify(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection, xcb_unmap_notify_event_t *ev)
{
    client_t *c;

    /*
     * event->send_event (Xlib)  is quivalent to  (ev->response_type &
     * 0x80)  in XCB  because the  SendEvent bit  is available  in the
     * response_type field
     */
    bool send_event = ((ev->response_type & 0x80) >> 7);

    if((c = client_get_bywin(globalconf.clients, ev->window))
       && ev->event == xcb_aux_get_screen(connection, c->phys_screen)->root
       && send_event && window_getstate(c->win) == XCB_WM_NORMAL_STATE)
        client_unmanage(c);

    return 0;
}

/** Handle XShape events
 * \param ev Shape event
 */
int
event_handle_shape(void *data __attribute__ ((unused)),
                   xcb_connection_t *connection __attribute__ ((unused)),
                   xcb_shape_notify_event_t *ev)
{
    client_t *c = client_get_bywin(globalconf.clients, ev->affected_window);

    if(c)
        window_setshape(c->win, c->phys_screen);

    return 0;
}

/** Handle XRandR events
 * \param ev RandrScreenChangeNotify event
 */
int
event_handle_randr_screen_change_notify(void *data __attribute__ ((unused)),
                                        xcb_connection_t *connection __attribute__ ((unused)),
                                        xcb_randr_screen_change_notify_event_t *ev)
{
    if(!globalconf.have_randr)
        return -1;

    /* Code  of  XRRUpdateConfiguration Xlib  function  ported to  XCB
     * (only the code relevant  to RRScreenChangeNotify) as the latter
     * doesn't provide this kind of function */
    if(ev->rotation & (XCB_RANDR_ROTATION_ROTATE_90 | XCB_RANDR_ROTATION_ROTATE_270))
        xcb_randr_set_screen_size(connection, ev->root, ev->height, ev->width,
                                  ev->mheight, ev->mwidth);
    else
        xcb_randr_set_screen_size(connection, ev->root, ev->width, ev->height,
                                  ev->mwidth, ev->mheight);

    /* XRRUpdateConfiguration also executes the following instruction
     * but it's not useful because SubpixelOrder is not used at all at
     * the moment
     *
     * XRenderSetSubpixelOrder(dpy, snum, scevent->subpixel_order);
     */

    uicb_restart(0, NULL);
    return 0;
}

/** Handle XClientMessage events
 * \param ev ClientMessage event
 */
int
event_handle_clientmessage(void *data __attribute__ ((unused)),
                           xcb_connection_t *connection __attribute__ ((unused)),
                           xcb_client_message_event_t *ev)
{
    ewmh_process_client_message(ev);
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
