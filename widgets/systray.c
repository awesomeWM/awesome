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
#include "screen.h"
#include "common/xembed.h"

extern awesome_t globalconf;

static int
systray_draw(draw_context_t *ctx,
             int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset, int used __attribute__ ((unused)),
             void *p)
{
    int i = 0, phys_screen;
    xembed_window_t *em;
    uint32_t config_win_vals[6];
    /* p is always a statusbar, titlebars are forbidden */
    statusbar_t *sb = (statusbar_t *) p;

    phys_screen = screen_virttophys(screen);

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
    config_win_vals[4] = sb->sw->window;
    /* stack mode */
    config_win_vals[5] = XCB_STACK_MODE_ABOVE;

    switch(sb->position)
    {
      case Left:
        config_win_vals[0] = sb->sw->geometry.x + w->area.y;
        config_win_vals[1] = sb->sw->geometry.y + sb->sw->geometry.height
            - w->area.x - config_win_vals[3];
        for(em = globalconf.embedded; em; em = em->next)
            if(config_win_vals[1] - config_win_vals[2] >= (uint32_t) sb->sw->geometry.y)
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
        config_win_vals[0] = sb->sw->geometry.x - w->area.y;
        config_win_vals[1] = sb->sw->geometry.y + w->area.x;
        for(em = globalconf.embedded; em; em = em->next)
            if(config_win_vals[1] + config_win_vals[3] <= (uint32_t) sb->sw->geometry.y + ctx->width)
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
        config_win_vals[0] = sb->sw->geometry.x + w->area.x;
        /* y */
        config_win_vals[1] = sb->sw->geometry.y + w->area.y;
    
        for(em = globalconf.embedded; em; em = em->next)
            /* if(x + width < systray.x + systray.width) */
            if(config_win_vals[0] + config_win_vals[2] <= (uint32_t) AREA_RIGHT(w->area) + sb->sw->geometry.x)
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

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
