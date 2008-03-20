/*
 * titlebar.c - titlebar management
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

#include <math.h>

#include "titlebar.h"
#include "screen.h"

extern AwesomeConf globalconf;

void
titlebar_init(Client *c)
{
    int titlebar_height = 1.5 * MAX(globalconf.screens[c->screen].styles.normal.font->height,
                                    MAX(globalconf.screens[c->screen].styles.focus.font->height,
                                        globalconf.screens[c->screen].styles.urgent.font->height));
    int phys_screen = get_phys_screen(c->screen);

    switch(c->titlebar.position)
    {
      case Top:
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          phys_screen,
                                          c->geometry.x,
                                          c->geometry.y - titlebar_height,
                                          c->geometry.width + 2 * c->border,
                                          titlebar_height,
                                          0);
        break;
      case Bottom:
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          phys_screen,
                                          c->geometry.x,
                                          c->geometry.y + c->geometry.height + 2 * c->border,
                                          c->geometry.width + 2 * c->border,
                                          titlebar_height,
                                          0);
        break;
      case Left:
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          phys_screen,
                                          c->geometry.x - titlebar_height,
                                          c->geometry.y,
                                          titlebar_height,
                                          c->geometry.width + 2 * c->border,
                                          0);
        break;
      case Right:
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          phys_screen,
                                          c->geometry.x + c->geometry.width + 2 * c->border,
                                          c->geometry.y,
                                          titlebar_height,
                                          c->geometry.width + 2 * c->border,
                                          0);
        break;
      case Off:
        break;
      default:
        c->titlebar.position = Off;
        break;
    }
}

void
titlebar_update(Client *c)
{
    Drawable dw = 0;
    DrawCtx *ctx;
    style_t style;
    area_t geometry;

    if(!c->titlebar.sw)
        return;

    switch(c->titlebar.position)
    {
      case Off:
        return;
      case Right:
      case Left:
        dw = XCreatePixmap(globalconf.display,
                           RootWindow(globalconf.display, c->titlebar.sw->phys_screen),
                           c->titlebar.sw->geometry.height,
                           c->titlebar.sw->geometry.width,
                           DefaultDepth(globalconf.display, c->titlebar.sw->phys_screen));
        ctx = draw_context_new(globalconf.display, c->titlebar.sw->phys_screen,
                               c->titlebar.sw->geometry.height,
                               c->titlebar.sw->geometry.width,
                               dw);
        geometry.width = c->titlebar.sw->geometry.height;
        geometry.height = c->titlebar.sw->geometry.width;
        break;
      default:
        ctx = draw_context_new(globalconf.display, c->titlebar.sw->phys_screen,
                               c->titlebar.sw->geometry.width,
                               c->titlebar.sw->geometry.height,
                               c->titlebar.sw->drawable);
        geometry = c->titlebar.sw->geometry;
        break;
    }


    if(c->isurgent)
        style = globalconf.screens[c->screen].styles.urgent;
    else if(globalconf.focus->client == c)
        style = globalconf.screens[c->screen].styles.focus;
    else
        style = globalconf.screens[c->screen].styles.normal;

    geometry.x = geometry.y = 0;

    draw_text(ctx, geometry, c->titlebar.text_align, style.font->height / 2,
              c->name, style);

    switch(c->titlebar.position)
    {
      case Left:
        draw_rotate(ctx, c->titlebar.sw->drawable, ctx->height, ctx->width,
                    - M_PI_2, 0, c->titlebar.sw->geometry.height);
        XFreePixmap(globalconf.display, dw);
        break;
      case Right:
        draw_rotate(ctx, c->titlebar.sw->drawable, ctx->height, ctx->width,
                    M_PI_2, c->titlebar.sw->geometry.width, 0);
        XFreePixmap(globalconf.display, dw);
      default:
        break;
    }

    simplewindow_refresh_drawable(c->titlebar.sw, c->titlebar.sw->phys_screen);

    draw_context_delete(ctx);
}

void
titlebar_update_geometry_floating(Client *c)
{
    if(!c->titlebar.sw)
        return;

    switch(c->titlebar.position)
    {
      default:
        return;
      case Top:
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x,
                                 c->geometry.y - c->titlebar.sw->geometry.height,
                                 c->geometry.width + 2 * c->border,
                                 c->titlebar.sw->geometry.height);
        break;
      case Bottom:
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x,
                                 c->geometry.y + c->geometry.height + 2 * c->border,
                                 c->geometry.width + 2 * c->border,
                                 c->titlebar.sw->geometry.height);
        break;
      case Left:
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x - c->titlebar.sw->geometry.width,
                                 c->geometry.y,
                                 c->titlebar.sw->geometry.width,
                                 c->geometry.height + 2 * c->border);
        break;
      case Right:
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + c->geometry.width + 2 * c->border,
                                 c->geometry.y,
                                 c->titlebar.sw->geometry.width,
                                 c->geometry.height + 2 * c->border);
        break;
    }

    titlebar_update(c);
}

area_t
titlebar_update_geometry(Client *c, area_t geometry)
{
    if(!c->titlebar.sw)
        return geometry;

    switch(c->titlebar.position)
    {
      default:
        return geometry;
      case Top:
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x,
                                 geometry.y,
                                 geometry.width + 2 * c->border,
                                 c->titlebar.sw->geometry.height);
        geometry.y += c->titlebar.sw->geometry.height;
        geometry.height -= c->titlebar.sw->geometry.height;
        break;
      case Bottom:
        geometry.height -= c->titlebar.sw->geometry.height;
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x,
                                 geometry.y + geometry.height + 2 * c->border,
                                 geometry.width + 2 * c->border,
                                 c->titlebar.sw->geometry.height);
        break;
      case Left:
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x,
                                 geometry.y,
                                 c->titlebar.sw->geometry.width,
                                 geometry.height + 2 * c->border);
        geometry.width -= c->titlebar.sw->geometry.width;
        geometry.x += c->titlebar.sw->geometry.width;
        break;
      case Right:
        geometry.width -= c->titlebar.sw->geometry.width;
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + geometry.width + 2 * c->border,
                                 geometry.y,
                                 c->titlebar.sw->geometry.width,
                                 geometry.height + 2 * c->border);
        break;
    }

    titlebar_update(c);

    return geometry;
}

/** Toggle window titlebar visibility
 * \param screen screen number (unused)
 * \param arg unused argument
 * \ingroup ui_callback
 */
void
uicb_client_toggletitlebar(int screen __attribute__ ((unused)),
                            char *arg __attribute__ ((unused)))
{
    Client *c = globalconf.focus->client;

    if(!c || !c->titlebar.sw)
        return;

    if(!c->titlebar.position)
        c->titlebar.position = c->titlebar.dposition;
    else
    {
        c->titlebar.position = Off;
        XUnmapWindow(globalconf.display, c->titlebar.sw->window);
    }

    globalconf.screens[c->screen].need_arrange = True;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
