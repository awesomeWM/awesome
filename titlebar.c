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
    int width;

    if(c->titlebar.position == Off
       || c->titlebar.position == Auto)
    {
        c->titlebar.position = Off;
        return;
    }

    if(!c->titlebar.height)
        c->titlebar.height = 1.5 * MAX(c->titlebar.styles.normal.font->height,
                                       MAX(c->titlebar.styles.focus.font->height,
                                           c->titlebar.styles.urgent.font->height));

    switch(c->titlebar.position)
    {
      case Top:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          c->phys_screen,
                                          c->geometry.x,
                                          c->geometry.y - c->titlebar.height,
                                          width,
                                          c->titlebar.height,
                                          0);
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          c->phys_screen,
                                          c->geometry.x,
                                          c->geometry.y + c->geometry.height + 2 * c->border,
                                          width,
                                          c->titlebar.height,
                                          0);
        break;
      case Left:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          c->phys_screen,
                                          c->geometry.x - c->titlebar.height,
                                          c->geometry.y,
                                          c->titlebar.height,
                                          width,
                                          0);
        break;
      case Right:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        c->titlebar.sw = simplewindow_new(globalconf.display,
                                          c->phys_screen,
                                          c->geometry.x + c->geometry.width + 2 * c->border,
                                          c->geometry.y,
                                          c->titlebar.height,
                                          width,
                                          0);
        break;
      default:
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
        style = c->titlebar.styles.urgent;
    else if(globalconf.focus->client == c)
        style = c->titlebar.styles.focus;
    else
        style = c->titlebar.styles.normal;

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

    draw_context_delete(&ctx);
}

void
titlebar_update_geometry_floating(Client *c)
{
    int width, x_offset = 0, y_offset = 0;

    if(!c->titlebar.sw)
        return;

    switch(c->titlebar.position)
    {
      default:
        return;
      case Top:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + c->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (c->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y - c->titlebar.sw->geometry.height,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + c->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (c->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y + c->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + c->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (c->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x - c->titlebar.sw->geometry.width,
                                 c->geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        break;
      case Right:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + c->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (c->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + c->geometry.width + 2 * c->border,
                                 c->geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        break;
    }

    titlebar_update(c);
}

area_t
titlebar_update_geometry(Client *c, area_t geometry)
{
    int width, x_offset = 0 , y_offset = 0;

    if(!c->titlebar.sw)
        return geometry;

    switch(c->titlebar.position)
    {
      default:
        return geometry;
      case Top:
        if(!c->titlebar.width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + x_offset,
                                 geometry.y,
                                 width,
                                 c->titlebar.sw->geometry.height);
        geometry.y += c->titlebar.sw->geometry.height;
        geometry.height -= c->titlebar.sw->geometry.height;
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        geometry.height -= c->titlebar.sw->geometry.height;
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + x_offset,
                                 geometry.y + geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar.width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x,
                                 geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        geometry.width -= c->titlebar.sw->geometry.width;
        geometry.x += c->titlebar.sw->geometry.width;
        break;
      case Right:
        if(!c->titlebar.width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        geometry.width -= c->titlebar.sw->geometry.width;
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + geometry.width + 2 * c->border,
                                 geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
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
