/*
 * systray.c - systray widget
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include "widget.h"
#include "common/xembed.h"

extern awesome_t globalconf;

typedef struct
{
    bool init;
} systray_data_t;

static bool
systray_init(void)
{
    xutil_intern_atom_request_t atom_systray_q, atom_manager_q;
    xcb_atom_t atom_systray;
    xcb_client_message_event_t ev;
    char atom_name[22];

    /* Send requests */
    atom_manager_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, atom_name);
    snprintf(atom_name, sizeof(atom_name), "_NET_SYSTEM_TRAY_S%d", globalconf.default_screen);
    atom_systray_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, atom_name);

    /* Fill event */
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[2] = globalconf.systray->sw->window;
    ev.data.data32[3] = ev.data.data32[4] = 0;
    ev.response_type = xutil_intern_atom_reply(globalconf.connection,
                                               &globalconf.atoms, atom_manager_q);

    ev.data.data32[1] = atom_systray = xutil_intern_atom_reply(globalconf.connection,
                                                               &globalconf.atoms,
                                                               atom_systray_q);

    xcb_set_selection_owner(globalconf.connection,
                            globalconf.systray->sw->window,
                            atom_systray,
                            XCB_CURRENT_TIME);

    return true;
}

static int
systray_draw(draw_context_t *ctx,
             int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset, int used __attribute__ ((unused)),
             void *p __attribute__ ((unused)))
{
    int i = 0;
    xembed_window_t *em;
    uint32_t config_win_vals[6];
    systray_data_t *d = w->widget->data;


    if(!d->init)
        d->init = systray_init();

    for(em = globalconf.embedded; em; em = em->next)
        i++;

    if(ctx->width - used > 0)
        w->area.width = MIN(i * ctx->height, ctx->width - used);
    else
        w->area.width = 0;
    w->area.height = ctx->height;
    w->area.x = widget_calculate_offset(ctx->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

    /* width */
    config_win_vals[2] = w->area.height;
    /* height */
    config_win_vals[3] = w->area.height;
    /* sibling */
    config_win_vals[4] = globalconf.systray->sw->window;
    /* stack mode */
    config_win_vals[5] = XCB_STACK_MODE_ABOVE;

    switch(globalconf.systray->position)
    {
      case Left:
        config_win_vals[0] = globalconf.systray->sw->geometry.x + w->area.y;
        config_win_vals[1] = globalconf.systray->sw->geometry.y + globalconf.systray->sw->geometry.height
            - w->area.x - config_win_vals[3];
        for(em = globalconf.embedded; em; em = em->next)
            if(config_win_vals[1] - config_win_vals[2] >= (uint32_t) globalconf.systray->sw->geometry.y)
            {
                xcb_map_window(globalconf.connection, em->win);
                xcb_configure_window(globalconf.connection, em->win,
                                     XCB_CONFIG_WINDOW_X
                                     | XCB_CONFIG_WINDOW_Y
                                     | XCB_CONFIG_WINDOW_WIDTH
                                     | XCB_CONFIG_WINDOW_HEIGHT
                                     | XCB_CONFIG_WINDOW_SIBLING
                                     | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[1] -= config_win_vals[3];
            }
            else
                xcb_unmap_window(globalconf.connection, em->win);
        break;
      case Right:
        config_win_vals[0] = globalconf.systray->sw->geometry.x - w->area.y;
        config_win_vals[1] = globalconf.systray->sw->geometry.y + w->area.x;
        for(em = globalconf.embedded; em; em = em->next)
            if(config_win_vals[1] + config_win_vals[3] <= (uint32_t) globalconf.systray->sw->geometry.y + ctx->width)
            {
                xcb_map_window(globalconf.connection, em->win);
                xcb_configure_window(globalconf.connection, em->win,
                                     XCB_CONFIG_WINDOW_X
                                     | XCB_CONFIG_WINDOW_Y
                                     | XCB_CONFIG_WINDOW_WIDTH
                                     | XCB_CONFIG_WINDOW_HEIGHT
                                     | XCB_CONFIG_WINDOW_SIBLING
                                     | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[1] += config_win_vals[3];
            }
            else
                xcb_unmap_window(globalconf.connection, em->win);
        break;
      default:
        /* x */
        config_win_vals[0] = globalconf.systray->sw->geometry.x + w->area.x;
        /* y */
        config_win_vals[1] = globalconf.systray->sw->geometry.y + w->area.y;
    
        for(em = globalconf.embedded; em; em = em->next)
            /* if(x + width < systray.x + systray.width) */
            if(config_win_vals[0] + config_win_vals[2] <= (uint32_t) AREA_RIGHT(w->area) + globalconf.systray->sw->geometry.x)
            {
                xcb_map_window(globalconf.connection, em->win);
                xcb_configure_window(globalconf.connection, em->win,
                                     XCB_CONFIG_WINDOW_X
                                     | XCB_CONFIG_WINDOW_Y
                                     | XCB_CONFIG_WINDOW_WIDTH
                                     | XCB_CONFIG_WINDOW_HEIGHT
                                     | XCB_CONFIG_WINDOW_SIBLING
                                     | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] += config_win_vals[2];
            }
            else
                xcb_unmap_window(globalconf.connection, em->win);
        break;
    }

    return w->area.width;
}

widget_t *
systray_new(alignment_t align)
{
    widget_t *w;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = systray_draw;
    w->cache_flags = WIDGET_CACHE_EMBEDDED;
    w->data = p_new(systray_data_t, 1);

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
